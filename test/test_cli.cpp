// Tests for SimOptionHandler / ParamOverlayGen command-line parsing,
// covering both happy paths (-p, -pin, -lp/-ls quit flags) and the
// SimOptionError exception hierarchy.

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "test_framework.hpp"

#include "Component.hpp"
#include "Param.hpp"
#include "ParamOverlay.hpp"
#include "SimOptionError.hpp"
#include "SimOptionHandler.hpp"


//// Helpers //////////////////////////////////////////////////////////////////

namespace {

// argv-style argument list whose underlying char buffers stay alive for the
// lifetime of the helper.  CommandOptionHandler::parseOptions takes char* (not
// const char*), and walks argv[i] mutably, so we can't pass a string literal
// directly.
class ArgList {
public:
    template <typename... Strings>
    explicit ArgList(Strings... args) {
        (push_(args), ...);
        rebuild_();
    }

    int    argc() const { return static_cast<int>(ptrs_.size()); }
    char **argv()       { return ptrs_.data(); }

private:
    std::vector<std::string>  storage_;
    std::vector<char *>       ptrs_;

    void push_(const char *s) { storage_.emplace_back(s); }
    void push_(const std::string &s) { storage_.push_back(s); }
    void rebuild_() {
        ptrs_.clear();
        ptrs_.reserve(storage_.size());
        for (auto &s : storage_) {
            ptrs_.push_back(s.data());
        }
    }
};

// A small Component with a couple of typed Params.  Tests construct one
// underneath their own TopLevelComponent so each test is independent of the
// singleton "sim" the SimOptionHandler also writes to.
class HostComp : public murm::Component {
public:
    murm::Param<int>         iparam { this, "iparam", 42,    "an int param" };
    murm::Param<double>      dparam { this, "dparam", 3.14,  "a double param" };
    murm::Param<std::string> sparam { this, "sparam", std::string("hello"),
                                      "a string param" };

    HostComp(murm::Component *parent, const std::string &name)
        : Component(parent, "HostComp", name) {}
};

// Write `contents` to a temp file and return its path.  The caller is
// responsible for std::remove()ing the file when done.
std::string write_temp_file(const std::string &name, const std::string &contents) {
    std::string path = std::string("/tmp/") + name;
    std::ofstream out(path);
    out << contents;
    return path;
}

} // namespace


//// Happy paths //////////////////////////////////////////////////////////////

TEST(parse_no_args_succeeds) {
    SimOptionHandler soh;
    ArgList args("prog");
    soh.parseSimOptions(args.argc(), args.argv());

    // With no -p, there should be no overrides waiting on the overlay.
    const murm::ParamOverlayNode *node = soh.getParamOverlayNode();
    ASSERT_TRUE(node != nullptr);
    ASSERT_TRUE(node->getOverrides().empty());
}

TEST(p_option_applies_override_to_local_root) {
    SimOptionHandler soh;
    ArgList args("prog", "-p", "sim.h.iparam=999");
    soh.parseSimOptions(args.argc(), args.argv());

    // Apply the parsed overlay to our own root - independent of the singleton.
    murm::TopLevelComponent root;
    root.addParamOverlayNode(soh.getParamOverlayNode());
    HostComp host(&root, "h");

    ASSERT_EQ(999, host.iparam.get());
    ASSERT_STREQ("999", host.iparam.getValStr());
    // Default string should still reflect the original default.
    ASSERT_STREQ("42", host.iparam.getDefaultValStr());
}

TEST(multiple_p_options_apply_to_distinct_params) {
    SimOptionHandler soh;
    ArgList args("prog",
                 "-p", "sim.h.iparam=10",
                 "-p", "sim.h.dparam=2.5",
                 "-p", "sim.h.sparam=world");
    soh.parseSimOptions(args.argc(), args.argv());

    murm::TopLevelComponent root;
    root.addParamOverlayNode(soh.getParamOverlayNode());
    HostComp host(&root, "h");

    ASSERT_EQ(10, host.iparam.get());
    ASSERT_NEAR(2.5, host.dparam.get(), 1e-12);
    ASSERT_STREQ("world", host.sparam.get());
}

TEST(p_option_supports_wildcard_param_name) {
    SimOptionHandler soh;
    ArgList args("prog", "-p", "sim.h.*param=12345");
    soh.parseSimOptions(args.argc(), args.argv());

    murm::TopLevelComponent root;
    root.addParamOverlayNode(soh.getParamOverlayNode());
    HostComp host(&root, "h");

    ASSERT_EQ(12345, host.iparam.get());
    ASSERT_NEAR(12345.0, host.dparam.get(), 1e-9);
    ASSERT_STREQ("12345", host.sparam.get());
}

TEST(pin_option_reads_overrides_from_file) {
    std::string path = write_temp_file(
        "murm_test_pin_ok.dat",
        "sim.h.iparam=77\n"
        "sim.h.dparam=1.25\n");

    SimOptionHandler soh;
    ArgList args("prog", "-pin", path);
    soh.parseSimOptions(args.argc(), args.argv());

    murm::TopLevelComponent root;
    root.addParamOverlayNode(soh.getParamOverlayNode());
    HostComp host(&root, "h");

    ASSERT_EQ(77, host.iparam.get());
    ASSERT_NEAR(1.25, host.dparam.get(), 1e-12);

    std::remove(path.c_str());
}

TEST(processCLA_returns_false_with_no_listing_options) {
    SimOptionHandler soh;
    ArgList args("prog");
    soh.parseSimOptions(args.argc(), args.argv());

    bool quit = soh.processCLA();
    ASSERT_FALSE(quit);
}

TEST(processCLA_returns_true_with_lp) {
    SimOptionHandler soh;
    ArgList args("prog", "-lp");
    soh.parseSimOptions(args.argc(), args.argv());

    // -lp prints an "Available parameters" listing for the singleton sim
    // tree, then asks main() to quit.  The listing itself is harmless (no
    // children attached to sim in this test binary), so we just check the
    // quit signal.
    bool quit = soh.processCLA();
    ASSERT_TRUE(quit);
}


//// Error paths /////////////////////////////////////////////////////////////

TEST(unknown_option_throws_UnknownOptionError) {
    SimOptionHandler soh;
    ArgList args("prog", "-zzz");

    ASSERT_THROWS(soh.parseSimOptions(args.argc(), args.argv()),
                  murm::UnknownOptionError);
}

TEST(unknown_option_message_includes_option_name) {
    SimOptionHandler soh;
    ArgList args("prog", "-zzz");

    bool caught = false;
    try {
        soh.parseSimOptions(args.argc(), args.argv());
    } catch (const murm::UnknownOptionError &e) {
        caught = true;
        std::string msg = e.what();
        ASSERT_TRUE(msg.find("zzz") != std::string::npos);
    }
    ASSERT_TRUE(caught);
}

TEST(p_spec_without_equals_throws_InvalidOverrideSpec) {
    SimOptionHandler soh;
    ArgList args("prog", "-p", "sim.h.iparam_no_equals");

    ASSERT_THROWS(soh.parseSimOptions(args.argc(), args.argv()),
                  murm::InvalidOverrideSpec);
}

TEST(pin_with_missing_file_throws_ParamFileNotFoundError) {
    SimOptionHandler soh;
    ArgList args("prog", "-pin", "/tmp/murm_test_does_not_exist_xyz.dat");

    ASSERT_THROWS(soh.parseSimOptions(args.argc(), args.argv()),
                  murm::ParamFileNotFoundError);
}

TEST(param_overlay_gen_throws_on_missing_equals) {
    murm::ParamOverlayGen gen;

    ASSERT_THROWS(gen.addOverride("no_equals_here"),
                  murm::InvalidOverrideSpec);

    // A well-formed spec on the same generator afterwards still works -
    // the generator's state isn't corrupted by a rejected addOverride.
    gen.addOverride("sim.h.iparam=7");
    murm::TopLevelComponent root;
    root.addParamOverlayNode(gen.getParamOverlay());
    HostComp host(&root, "h");
    ASSERT_EQ(7, host.iparam.get());
}


//// Exception hierarchy //////////////////////////////////////////////////////

TEST(specific_errors_catchable_as_SimOptionError) {
    SimOptionHandler soh;
    ArgList args("prog", "-p", "no_equals");

    bool caught_as_base = false;
    try {
        soh.parseSimOptions(args.argc(), args.argv());
    } catch (const murm::SimOptionError &) {
        caught_as_base = true;
    }
    ASSERT_TRUE(caught_as_base);
}

TEST(SimOptionError_catchable_as_std_runtime_error) {
    SimOptionHandler soh;
    ArgList args("prog", "-bogus");

    bool caught_as_runtime_error = false;
    try {
        soh.parseSimOptions(args.argc(), args.argv());
    } catch (const std::runtime_error &) {
        caught_as_runtime_error = true;
    }
    ASSERT_TRUE(caught_as_runtime_error);
}


RUN_ALL_TESTS()
