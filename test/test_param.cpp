// Tests for Param<T> and the Component parameter registry.

#include <string>
#include <vector>

#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "Component.hpp"
#include "Param.hpp"
#include "ParamOverlay.hpp"


class HostComp : public murm::Component {
public:
    murm::Param<int>         iparam   { this, "iparam",   42,    "an int param" };
    murm::Param<double>      dparam   { this, "dparam",   3.14,  "a double param" };
    murm::Param<std::string> sparam   { this, "sparam",   std::string("hello"), "a string param" };
    murm::Param<unsigned>    uparam   { this, "uparam",   7u,    "an unsigned param" };

    HostComp(murm::Component *parent, const std::string &name)
        : Component(parent, "HostComp", name) {}
};


//// Default values, get(), conversion operator ///////////////////////////////

TEST(default_values_visible_via_get_and_conversion) {
    murm::TopLevelComponent root;
    HostComp host(&root, "h");

    ASSERT_EQ(42, host.iparam.get());
    int as_int = host.iparam;       // implicit conversion operator
    ASSERT_EQ(42, as_int);

    ASSERT_NEAR(3.14, host.dparam.get(), 1e-12);

    const std::string &s = host.sparam.get();
    ASSERT_STREQ("hello", s);

    ASSERT_EQ((unsigned)7, host.uparam.get());
}


//// Names and full names /////////////////////////////////////////////////////

TEST(param_names_and_full_names) {
    murm::TopLevelComponent root;
    HostComp host(&root, "h");

    ASSERT_STREQ("iparam", host.iparam.getName());
    ASSERT_STREQ("sim.h.iparam", host.iparam.getFullName());
    ASSERT_STREQ("an int param", host.iparam.getDescrip());

    ASSERT_STREQ("sim.h.dparam", host.dparam.getFullName());
    ASSERT_STREQ("sim.h.sparam", host.sparam.getFullName());
}

TEST(param_default_value_string) {
    murm::TopLevelComponent root;
    HostComp host(&root, "h");

    ASSERT_STREQ("42", host.iparam.getDefaultValStr());
    ASSERT_STREQ("hello", host.sparam.getDefaultValStr());
    ASSERT_STREQ("7", host.uparam.getDefaultValStr());
}

TEST(param_value_string_matches_default_initially) {
    murm::TopLevelComponent root;
    HostComp host(&root, "h");

    ASSERT_STREQ("42", host.iparam.getValStr());
    ASSERT_STREQ("hello", host.sparam.getValStr());
}


//// set() updates value and value string /////////////////////////////////////

TEST(param_set_updates_value) {
    murm::TopLevelComponent root;
    HostComp host(&root, "h");

    host.iparam.set(100);
    ASSERT_EQ(100, host.iparam.get());
    ASSERT_STREQ("100", host.iparam.getValStr());
    // Default string should be untouched
    ASSERT_STREQ("42", host.iparam.getDefaultValStr());

    host.sparam.set("world");
    ASSERT_STREQ("world", host.sparam.get());
    ASSERT_STREQ("world", host.sparam.getValStr());
}


//// Component param registry /////////////////////////////////////////////////

TEST(component_getParams_lists_all_params_in_declaration_order) {
    murm::TopLevelComponent root;
    HostComp host(&root, "h");

    const auto &params = host.getParams();
    ASSERT_EQ((size_t)4, params.size());
    ASSERT_STREQ("iparam",  params[0]->getName());
    ASSERT_STREQ("dparam",  params[1]->getName());
    ASSERT_STREQ("sparam",  params[2]->getName());
    ASSERT_STREQ("uparam",  params[3]->getName());
}


//// Parameter overlay overrides //////////////////////////////////////////////

TEST(param_overlay_overrides_default_value) {
    murm::ParamOverlayGen gen;
    // Override sim.h.iparam from its default (42) to 999.
    gen.addOverride("sim.h.iparam=999");

    murm::TopLevelComponent root;
    root.addParamOverlayNode(gen.getParamOverlay());
    HostComp host(&root, "h");

    ASSERT_EQ(999, host.iparam.get());
    ASSERT_STREQ("999", host.iparam.getValStr());
}

TEST(param_overlay_wildcard_match_on_param_name) {
    // Match every param ending in "param" on sim.h.
    murm::ParamOverlayGen gen;
    gen.addOverride("sim.h.*param=12345");

    murm::TopLevelComponent root;
    root.addParamOverlayNode(gen.getParamOverlay());
    HostComp host(&root, "h");

    ASSERT_EQ(12345,    host.iparam.get());
    ASSERT_NEAR(12345.0, host.dparam.get(), 1e-9);
    ASSERT_EQ((unsigned)12345, host.uparam.get());
}

TEST(param_overlay_does_not_override_when_path_doesnt_match) {
    murm::ParamOverlayGen gen;
    gen.addOverride("sim.other.iparam=999");   // different component

    murm::TopLevelComponent root;
    root.addParamOverlayNode(gen.getParamOverlay());
    HostComp host(&root, "h");

    ASSERT_EQ(42, host.iparam.get());          // untouched default
}


RUN_ALL_TESTS()
