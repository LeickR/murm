// Tests for Component: construction, naming, hierarchy, iteration.

#include <string>
#include <vector>

#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "Component.hpp"


// A minimal concrete Component subclass we can instantiate at will.
class TestComp : public murm::Component {
public:
    TestComp(murm::Component *parent, const std::string &type_name, const std::string &name)
        : Component(parent, type_name, name) {}
    TestComp(murm::Component *parent, const std::string &type_name, int index)
        : Component(parent, type_name, index) {}
};


//// Naming and validation ////////////////////////////////////////////////////

TEST(isValidName_accepts_normal_names) {
    ASSERT_TRUE(murm::Component::isValidName("foo"));
    ASSERT_TRUE(murm::Component::isValidName("foo_bar"));
    ASSERT_TRUE(murm::Component::isValidName("foo123"));
    ASSERT_TRUE(murm::Component::isValidName("Node[0]"));   // square brackets are allowed
    ASSERT_TRUE(murm::Component::isValidName(""));          // empty is technically allowed
}

TEST(isValidName_rejects_special_chars) {
    ASSERT_FALSE(murm::Component::isValidName("a.b"));
    ASSERT_FALSE(murm::Component::isValidName("a*b"));
    ASSERT_FALSE(murm::Component::isValidName("a?b"));
    ASSERT_FALSE(murm::Component::isValidName("a=b"));
}


//// Hierarchy and full names /////////////////////////////////////////////////

TEST(child_full_name_uses_parent_path) {
    murm::TopLevelComponent root;
    TestComp parent(&root, "Parent", "p");
    TestComp child(&parent, "Child", "c");
    ASSERT_STREQ("sim.p", parent.getFullName());
    ASSERT_STREQ("sim.p.c", child.getFullName());
    ASSERT_STREQ("p", parent.getName());
    ASSERT_STREQ("c", child.getName());
    ASSERT_STREQ("Parent", parent.getTypeName());
    ASSERT_STREQ("Child", child.getTypeName());
}

TEST(indexed_constructor_builds_bracket_name) {
    murm::TopLevelComponent root;
    TestComp parent(&root, "Parent", "p");
    TestComp child0(&parent, "Child", 0);
    TestComp child7(&parent, "Child", 7);
    ASSERT_STREQ("Child[0]", child0.getName());
    ASSERT_STREQ("Child[7]", child7.getName());
    ASSERT_STREQ("sim.p.Child[0]", child0.getFullName());
    ASSERT_STREQ("sim.p.Child[7]", child7.getFullName());
}

TEST(grandchild_full_name) {
    murm::TopLevelComponent root;
    TestComp a(&root, "A", "a");
    TestComp b(&a, "B", "b");
    TestComp c(&b, "C", "c");
    ASSERT_STREQ("sim.a.b.c", c.getFullName());
}


//// Tree iteration ///////////////////////////////////////////////////////////

TEST(iterator_visits_self_only_for_leaf) {
    murm::TopLevelComponent root;
    TestComp leaf(&root, "Leaf", "leaf");

    std::vector<std::string> visited;
    for (auto &comp : leaf) {
        visited.push_back(comp.getName());
    }
    ASSERT_EQ((size_t)1, visited.size());
    ASSERT_STREQ("leaf", visited[0]);
}

TEST(iterator_pre_order_dfs) {
    // Tree:
    //   root
    //   ├── a
    //   │   ├── a0
    //   │   └── a1
    //   └── b
    murm::TopLevelComponent root;
    TestComp a(&root, "Branch", "a");
    TestComp a0(&a, "Leaf", "a0");
    TestComp a1(&a, "Leaf", "a1");
    TestComp b(&root, "Branch", "b");

    std::vector<std::string> order;
    for (auto &comp : root) {
        order.push_back(comp.getName());
    }
    ASSERT_EQ((size_t)5, order.size());
    ASSERT_STREQ("sim",  order[0]);   // TopLevelComponent itself
    ASSERT_STREQ("a",    order[1]);
    ASSERT_STREQ("a0",   order[2]);
    ASSERT_STREQ("a1",   order[3]);
    ASSERT_STREQ("b",    order[4]);
}

TEST(begin_end_inequality_for_nonempty_tree) {
    murm::TopLevelComponent root;
    TestComp leaf(&root, "Leaf", "leaf");
    ASSERT_TRUE(root.begin() != root.end());
}


//// Stats registration ///////////////////////////////////////////////////////

TEST(getStat_returns_nullptr_for_unknown_name) {
    murm::TopLevelComponent root;
    TestComp comp(&root, "Comp", "c");
    ASSERT_TRUE(comp.getStat("does_not_exist") == nullptr);
}


RUN_ALL_TESTS()
