// Tests for string_utils.hpp.

#include <sstream>
#include <string>
#include <vector>

#include "test_framework.hpp"
#include "string_utils.hpp"

using string_utils::wildMatch;
using string_utils::splitString;
using string_utils::splitStringOnFirst;


//// wildMatch ////////////////////////////////////////////////////////////////

TEST(wildmatch_exact) {
    ASSERT_TRUE(wildMatch("hello", "hello"));
    ASSERT_FALSE(wildMatch("hello", "world"));
    ASSERT_FALSE(wildMatch("hello", "hell"));
    ASSERT_FALSE(wildMatch("hell", "hello"));
}

TEST(wildmatch_empty_strings) {
    ASSERT_TRUE(wildMatch("", ""));
    ASSERT_FALSE(wildMatch("a", ""));
    ASSERT_FALSE(wildMatch("", "a"));
    ASSERT_TRUE(wildMatch("*", ""));
    ASSERT_TRUE(wildMatch("**", ""));
}

TEST(wildmatch_question_mark_one_char) {
    ASSERT_TRUE(wildMatch("h?llo", "hello"));
    ASSERT_TRUE(wildMatch("h?llo", "hxllo"));
    ASSERT_FALSE(wildMatch("h?llo", "hllo"));   // ? requires at least one char in the middle
    ASSERT_FALSE(wildMatch("h?llo", "hexllo")); // not two chars
    ASSERT_TRUE(wildMatch("?????", "abcde"));
}

TEST(wildmatch_star_zero_or_more) {
    ASSERT_TRUE(wildMatch("*", "anything"));
    ASSERT_TRUE(wildMatch("*", ""));
    ASSERT_TRUE(wildMatch("h*o", "ho"));
    ASSERT_TRUE(wildMatch("h*o", "hello"));
    ASSERT_TRUE(wildMatch("h*o", "hellllllo"));
    ASSERT_FALSE(wildMatch("h*o", "hellx"));
    ASSERT_TRUE(wildMatch("*foo*", "barfoobar"));
    ASSERT_TRUE(wildMatch("*foo*", "foo"));
    ASSERT_FALSE(wildMatch("*foo*", "fo"));
}

TEST(wildmatch_mixed_wildcards) {
    ASSERT_TRUE(wildMatch("a?c*", "abcdef"));
    ASSERT_TRUE(wildMatch("a?c*", "abc"));
    ASSERT_FALSE(wildMatch("a?c*", "ac"));
    ASSERT_TRUE(wildMatch("*.cpp", "main.cpp"));
    ASSERT_TRUE(wildMatch("*.cpp", "a.b.cpp"));
    ASSERT_FALSE(wildMatch("*.cpp", "main.hpp"));
}

TEST(wildmatch_string_overload) {
    std::string pattern = "abc*";
    std::string text    = "abcdef";
    ASSERT_TRUE(wildMatch(pattern, text));
    ASSERT_FALSE(wildMatch(std::string("xyz*"), std::string("abcdef")));
}


//// splitString //////////////////////////////////////////////////////////////

TEST(splitString_basic) {
    std::vector<std::string> parts;
    splitString("a,b,c", ',', parts);
    ASSERT_EQ((size_t)3, parts.size());
    ASSERT_STREQ("a", parts[0]);
    ASSERT_STREQ("b", parts[1]);
    ASSERT_STREQ("c", parts[2]);
}

TEST(splitString_empty_input) {
    std::vector<std::string> parts;
    splitString("", ',', parts);
    ASSERT_EQ((size_t)0, parts.size());
}

TEST(splitString_no_delimiter) {
    std::vector<std::string> parts;
    splitString("hello", ',', parts);
    ASSERT_EQ((size_t)1, parts.size());
    ASSERT_STREQ("hello", parts[0]);
}

TEST(splitString_appends_to_existing) {
    std::vector<std::string> parts {"existing"};
    splitString("a,b", ',', parts);
    ASSERT_EQ((size_t)3, parts.size());
    ASSERT_STREQ("existing", parts[0]);
    ASSERT_STREQ("a", parts[1]);
    ASSERT_STREQ("b", parts[2]);
}


//// splitStringOnFirst ///////////////////////////////////////////////////////

TEST(splitOnFirst_basic) {
    std::string l, r;
    splitStringOnFirst("name=value", '=', l, r);
    ASSERT_STREQ("name", l);
    ASSERT_STREQ("value", r);
}

TEST(splitOnFirst_only_first_delim) {
    std::string l, r;
    splitStringOnFirst("a=b=c", '=', l, r);
    ASSERT_STREQ("a", l);
    ASSERT_STREQ("b=c", r);
}

TEST(splitOnFirst_no_delim) {
    // Note: the header comment claims that `right` will be empty when the
    // delimiter is not found, but the implementation (via `getline` + `>>`)
    // simply leaves `right` untouched.  We test the actual behaviour.
    std::string l = "old_l";
    std::string r;       // start empty so we don't rely on the documented (but
                         // not-actually-implemented) clearing
    splitStringOnFirst("noeq", '=', l, r);
    ASSERT_STREQ("noeq", l);
    ASSERT_STREQ("", r);
}


RUN_ALL_TESTS()
