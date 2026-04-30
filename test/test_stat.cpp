// Tests for StatSampler.

#include <string>
#include <vector>
#include <cmath>

#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "Component.hpp"
#include "Stat.hpp"


class StatHost : public murm::Component {
public:
    murm::StatSampler latency  { this, "latency",  "Per-sample latency (full sampler)" };
    murm::StatSampler hits     { this, "hits",     "Hit counter (accumulator)",
                                 murm::StatSampler::IS_ACCUMULATOR };

    StatHost(murm::Component *parent, const std::string &name)
        : Component(parent, "StatHost", name) {}
};


//// Empty sampler ////////////////////////////////////////////////////////////

TEST(empty_sampler_initial_state) {
    murm::StatSampler s;
    ASSERT_EQ((uint64_t)0, s.getCount());
    ASSERT_NEAR(0.0, s.getSum(),     1e-12);
    ASSERT_NEAR(0.0, s.getMean(),    1e-12);
    ASSERT_NEAR(0.0, s.getStdDev(),  1e-12);
    ASSERT_FALSE(s.isMinValid());
}


//// Sample accumulation //////////////////////////////////////////////////////

TEST(addSample_updates_count_and_sum) {
    murm::StatSampler s;
    s.addSample(1.0);
    s.addSample(2.0);
    s.addSample(3.0);
    ASSERT_EQ((uint64_t)3, s.getCount());
    ASSERT_NEAR(6.0, s.getSum(),  1e-12);
    ASSERT_NEAR(2.0, s.getMean(), 1e-12);
}

TEST(addSample_tracks_min_max) {
    murm::StatSampler s;
    s.addSample(5.0);
    s.addSample(2.0);
    s.addSample(7.0);
    s.addSample(4.0);
    ASSERT_TRUE(s.isMinValid());
    ASSERT_NEAR(2.0, s.getMin(), 1e-12);
    ASSERT_NEAR(7.0, s.getMax(), 1e-12);
}

TEST(stddev_of_constant_samples_is_zero) {
    murm::StatSampler s;
    for (int i = 0; i < 10; ++i) s.addSample(5.0);
    ASSERT_NEAR(5.0, s.getMean(),   1e-12);
    ASSERT_NEAR(0.0, s.getStdDev(), 1e-12);
}

TEST(stddev_population_formula) {
    // StatSampler computes population stddev: sqrt(E[X^2] - E[X]^2).
    // For samples 1, 3:  mean=2, E[X^2]=5, var=1, std=1.
    murm::StatSampler s;
    s.addSample(1.0);
    s.addSample(3.0);
    ASSERT_NEAR(2.0, s.getMean(),   1e-12);
    ASSERT_NEAR(1.0, s.getStdDev(), 1e-12);
}


//// Increment / += operators /////////////////////////////////////////////////

TEST(prefix_increment_adds_one_sample_of_value_one) {
    murm::StatSampler s;
    ++s;
    ++s;
    ++s;
    ASSERT_EQ((uint64_t)3, s.getCount());
    ASSERT_NEAR(3.0, s.getSum(), 1e-12);
}

TEST(plus_equals_double_is_addSample) {
    murm::StatSampler s;
    s += 4.0;
    s += 6.0;
    ASSERT_EQ((uint64_t)2, s.getCount());
    ASSERT_NEAR(10.0, s.getSum(), 1e-12);
}


//// Reset ////////////////////////////////////////////////////////////////////

TEST(reset_clears_state) {
    murm::StatSampler s;
    s.addSample(5.0);
    s.addSample(7.0);
    s.reset();
    ASSERT_EQ((uint64_t)0, s.getCount());
    ASSERT_NEAR(0.0, s.getSum(), 1e-12);
    ASSERT_FALSE(s.isMinValid());
}


//// Sampler arithmetic ///////////////////////////////////////////////////////

TEST(operator_plus_combines_two_samplers) {
    murm::StatSampler a;
    a.addSample(1.0); a.addSample(2.0);

    murm::StatSampler b;
    b.addSample(3.0); b.addSample(4.0);

    murm::StatSampler combined = a + b;
    ASSERT_EQ((uint64_t)4, combined.getCount());
    ASSERT_NEAR(10.0, combined.getSum(), 1e-12);
    ASSERT_NEAR(1.0,  combined.getMin(), 1e-12);
    ASSERT_NEAR(4.0,  combined.getMax(), 1e-12);
}

TEST(operator_plus_equals_combines_in_place) {
    murm::StatSampler a;
    a.addSample(1.0);

    murm::StatSampler b;
    b.addSample(5.0); b.addSample(6.0);

    a += b;
    ASSERT_EQ((uint64_t)3, a.getCount());
    ASSERT_NEAR(12.0, a.getSum(), 1e-12);
    ASSERT_NEAR(1.0,  a.getMin(), 1e-12);
    ASSERT_NEAR(6.0,  a.getMax(), 1e-12);
}


//// Accumulator-mode sampler /////////////////////////////////////////////////

TEST(accumulator_sampler_skips_min_max_tracking) {
    murm::StatSampler acc(/*owner=*/nullptr, "acc", "accumulator",
                          murm::StatSampler::IS_ACCUMULATOR);
    acc.addSample(2.0);
    acc.addSample(5.0);
    acc.addSample(1.0);
    ASSERT_TRUE(acc.isAccumulator());
    ASSERT_EQ((uint64_t)3, acc.getCount());
    ASSERT_NEAR(8.0, acc.getSum(), 1e-12);
    // min/max are not maintained for accumulators.
}


//// Component-attached sampler ///////////////////////////////////////////////

TEST(component_attached_sampler_has_full_name_and_is_registered) {
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());
    StatHost host(&root, "host");

    ASSERT_STREQ("latency", host.latency.getName());
    ASSERT_STREQ("sim.host.latency", host.latency.getFullName());
    ASSERT_STREQ("Per-sample latency (full sampler)", host.latency.getDescrip());

    ASSERT_FALSE(host.latency.isAccumulator());
    ASSERT_TRUE(host.hits.isAccumulator());

    // Both samplers should be registered with the component.
    ASSERT_EQ((size_t)2, host.getStats().size());
    ASSERT_TRUE(host.getStat("latency") != nullptr);
    ASSERT_TRUE(host.getStat("hits")    != nullptr);
    ASSERT_STREQ("sim.host.latency", host.getStat("latency")->getFullName());
    ASSERT_TRUE(host.getStat("does_not_exist") == nullptr);
}


//// Single-value convenience constructor /////////////////////////////////////

TEST(double_constructor_creates_one_sample_sampler) {
    murm::StatSampler s(42.0);
    ASSERT_EQ((uint64_t)1, s.getCount());
    ASSERT_NEAR(42.0, s.getSum(), 1e-12);
    ASSERT_NEAR(42.0, s.getMin(), 1e-12);
    ASSERT_NEAR(42.0, s.getMax(), 1e-12);
}


RUN_ALL_TESTS()
