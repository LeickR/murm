
#ifndef __STAT_UTILS_HRR_HPP__
#define __STAT_UTILS_HRR_HPP__

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <string>
#include <vector>
#include "../include/Stat.hpp"
#include "../include/Component.hpp"


typedef std::vector<std::pair<double, std::pair<double, double>>> MinMaxTimeSequenceVec;


// Utilities for outputting stat sequences (e.g., for plotting)
// Output the sum:
inline void outputStatSequence_Sum(simian::StatSamplerTimeSequenceVec &sampler_seq,
                                              const std::string &tag,
                                              const std::string &header)
{
    std::cout << header << std::endl;
    std::cout << "Time:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.first << "\t"; } std::cout << std::endl;
    std::cout << tag << "\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.second.getSum() << "\t"; } std::cout << std::endl;
}

// Output the mean:
inline void outputStatSequence_Mean(simian::StatSamplerTimeSequenceVec &sampler_seq,
                                                const std::string &tag,
                                                const std::string &header)
{
    std::cout << header << std::endl;
    std::cout << "Time:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.first << "\t"; } std::cout << std::endl;
    std::cout << tag << "\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.second.getMean() << "\t"; } std::cout << std::endl;
}

// Output the mean, standard deviation, min, max, etc.:
inline void outputStatSequence_MeanStdDev(simian::StatSamplerTimeSequenceVec &sampler_seq,
                                                        const std::string &tag,
                                                        const std::string &header)
{
    std::cout << header << std::endl;
    std::cout << "Time:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.first << "\t"; } std::cout << std::endl;
    std::cout << tag << " Mean:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.second.getMean() << "\t"; } std::cout << std::endl;
    std::cout << tag << " Std_dev:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.second.getStdDev() << "\t"; } std::cout << std::endl;
    std::cout << tag << " -sigma:\t";
    for (const auto &pair : sampler_seq) { std::cout << (pair.second.getMean() - pair.second.getStdDev())
                                                                     << "\t"; } std::cout << std::endl;
    std::cout << tag << " +sigma:\t";
    for (const auto &pair : sampler_seq) { std::cout << (pair.second.getMean() + pair.second.getStdDev())
                                                                     << "\t"; } std::cout << std::endl;
}

// Output the mean, standard deviation, min, max, etc.:
inline void outputStatSequence_MeanStdDevMinMax(simian::StatSamplerTimeSequenceVec &sampler_seq,
                                                                const std::string &tag,
                                                                const std::string &header)
{
    std::cout << header << std::endl;
    std::cout << "Time:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.first << "\t"; } std::cout << std::endl;
    std::cout << tag << " Mean:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.second.getMean() << "\t"; } std::cout << std::endl;
    std::cout << tag << " Std_dev:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.second.getStdDev() << "\t"; } std::cout << std::endl;
    std::cout << tag << " -sigma:\t";
    for (const auto &pair : sampler_seq) { std::cout << (pair.second.getMean() - pair.second.getStdDev())
                                                                     << "\t"; } std::cout << std::endl;
    std::cout << tag << " +sigma:\t";
    for (const auto &pair : sampler_seq) { std::cout << (pair.second.getMean() + pair.second.getStdDev())
                                                                     << "\t"; } std::cout << std::endl;

    //for (const auto &pair : sampler_seq) {
    //   assert(pair.second.isAccumulator() == false);
    //   assert(pair.second.isMaxValid());
    //}

    std::cout << tag << " Min:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.second.getMin() << "\t"; } std::cout << std::endl;
    std::cout << tag << " Max:\t";
    for (const auto &pair : sampler_seq) { std::cout << pair.second.getMax() << "\t"; } std::cout << std::endl;
}


// Utility class for collecting averaged stats over the stat window
class StatWindow {
public:
    enum StatType {
        MEAN,           // The mean value of all samples over the stat window
        MEAN_SUM_DERIV, // The change in the average sum per component over the change in time
        MEAN_CUM_DERIV, // The average change in a (cumulative) value over the change in time
        COUNT_PER_COMP  // The average sample count per component (at the stop time)
    };
private:
    struct StatTracker {
        std::string stat_name;
        StatType stat_type;
        std::string tag;
        std::string descrip;
        simian::StatSampler accum; // Temporary accumulator used at the start/stop points
        simian::StatSampler start_state;
        simian::StatSampler final_state;
        double min {0.0};
        double max {0.0};

        // Constructor
        StatTracker(const std::string &st_nm, StatType st_tp,
                        const std::string &tg, const std::string &desc) :
            stat_name(st_nm),
            stat_type(st_tp),
            tag(tg),
            descrip(desc)
        {}

        void resetTemp() { accum.reset(); }

        void accumulate(const std::map<std::string, simian::StatSampler*> &statmap) {
            switch (stat_type) {
            case MEAN:
            case MEAN_CUM_DERIV:
            case COUNT_PER_COMP:
                accum += *(statmap.at(stat_name));
                break;
            case MEAN_SUM_DERIV:
                accum.addSample(statmap.at(stat_name)->getSum());
                break;
            default:
                assert(0);
            }
        }

        // After accumulation, finish all time window start point computations
        void computeStartState() {
            switch (stat_type) {
            case MEAN:
            case MEAN_SUM_DERIV:
            case MEAN_CUM_DERIV:
                start_state = accum;
                break;
            case COUNT_PER_COMP:
                // Nothing to do here
                break;
            default:
                assert(0);
            }
        }
        
        // After accumulation, finish all time window stop point computations
        void computeStopState(double time_interval, uint comp_count) {
            switch (stat_type) {
            case MEAN:
                final_state = (accum - start_state);
                min = accum.getMin();
                max = accum.getMax();
                break;
            case MEAN_SUM_DERIV:
            case MEAN_CUM_DERIV:
                {
                    double change_over_time = (accum.getMean() - start_state.getMean())/time_interval;
                    final_state.addSample(change_over_time);
                    break;
                }
            case COUNT_PER_COMP:
                {
                    double count_per_comp = ((double)accum.getCount())/comp_count;
                    final_state.addSample(count_per_comp);
                    break;
                }
            default:
                assert(0);
            }
        }

        // Output the final results
        void printResults() {
            std::cout << "\n";
            switch (stat_type) {
            case MEAN:
            case MEAN_CUM_DERIV:
                std::cout << descrip << "\n" << tag << " mean" << "\t" << final_state.getMean() << std::endl;
                //std::cout << tag << " std_dev" << "\t" << final_state.getStdDev() << std::endl;
                std::cout << tag << " min (over entire range)" << "\t" << min << std::endl;
                std::cout << tag << " max (over entire range)" << "\t" << max << std::endl;
                break;
            case MEAN_SUM_DERIV:
            case COUNT_PER_COMP:
                std::cout << descrip << "\n" << tag << "\t" << final_state.getSum() << std::endl;
                break;
            default:
                assert(0);
            }
        }
    };

    std::vector<StatTracker> stat_trackers_;
    uint comp_count_ {0};

public:
    // Add a stat to track
    void addStat(const std::string &stat_name, StatType stat_type,
                     const std::string &tag, const std::string &descrip)
    {
        stat_trackers_.emplace_back(stat_name, stat_type, tag, descrip);
    }

    // Reset all temporary variables prior to accumulation for this time point
    void resetTemp() {
        comp_count_ = 0;

        // Reset any temporary variables used in the StatTrackers
        for (auto &stat_tracker : stat_trackers_) { stat_tracker.resetTemp(); }
    }

    // Accumulate stats at the start and end of the time window (for this component)
    void accumulate(simian::Component &comp) {
        ++comp_count_;
        
        // Update any "snapshot" stats
        comp.computeStatSnapshot();
        
        // Accumulate the data from this Component for each of the StatTrackers
        const std::map<std::string, simian::StatSampler*> &statmap = comp.getStats();
        for (auto &stat_tracker : stat_trackers_) { stat_tracker.accumulate(statmap); }
    }

    // After accumulation, finish all time window start point computations
    void computeStartState() {
        for (auto &stat_tracker : stat_trackers_) { stat_tracker.computeStartState(); }
    }

    // After accumulation, finish all time window stop point computations
    void computeStopState(double time_interval) {
        for (auto &stat_tracker : stat_trackers_) { stat_tracker.computeStopState(time_interval, comp_count_); }
    }

    // Print all results
    void printResults() {
        for (auto &stat_tracker : stat_trackers_) { stat_tracker.printResults(); }
    }
};


// Utility class for collecting stat time sequences
class StatSeqs {
public:
    enum StatType {
        MEAN,           // The mean value of all samples collected during each interval
        MEAN_SNAP,      // The mean value of all samples for a "snapshot" stat
        MEAN_SUM_DERIV, // The change in the average sum per component over the change in time
        MEAN_CUM_DERIV, // The average change in a (cumulative) value over the change in time
        MEAN_ACTIVE_PER_COMP  // For a stat measuring a cumulative "active" time value for some resource,
                                     // determines the average number of active resources per component
    };
private:
    struct StatSeq {
        std::string stat_name;
        StatType stat_type;
        std::string tag;
        std::string descrip;
        simian::StatSampler accum; // Temporary accumulator used at each time point
        simian::StatSampler last_state;
        simian::StatSamplerTimeSequenceVec time_seq_vec;
        MinMaxTimeSequenceVec min_max_seq_vec;
        
        // Constructor
        StatSeq(const std::string &st_nm, StatType st_tp,
                  const std::string &tg, const std::string &desc) :
            stat_name(st_nm),
            stat_type(st_tp),
            tag(tg),
            descrip(desc)
        {}
        
        void resetTemp() {
            accum.reset();

            // Debug
            //if (stat_name == "fraction_idle_wl") {
            //   std::cout << "StatSeq::resetTemp(): " << stat_name << ": accum mean=" << accum.getMean()
            //             << ", sum=" << accum.getSum() << ", count=" << accum.getCount() << std::endl;
            //}
        }

        void accumulate(const std::map<std::string, simian::StatSampler*> &statmap) {
            switch (stat_type) {
            case MEAN:
            case MEAN_SNAP:
            case MEAN_CUM_DERIV:
            case MEAN_ACTIVE_PER_COMP:
                // Debug
                //if (stat_name == "fraction_idle_wl") {
                //   std::cout << "StatSeq::accumulate(): " << stat_name << ": accum(before) mean=" << accum.getMean()
                //             << ", sum=" << accum.getSum() << ", count=" << accum.getCount() << std::endl;
                //   std::cout << "StatSeq::accumulate(): " << stat_name << ": stat mean=" << (statmap.at(stat_name))->getMean()
                //             << ", sum=" << (statmap.at(stat_name))->getSum() << ", count=" << (statmap.at(stat_name))->getCount() << std::endl;
                //}
                accum += *(statmap.at(stat_name));
                // Debug
                //if (stat_name == "fraction_idle_wl") {
                //   std::cout << "StatSeq::accumulate(): " << stat_name << ": accum(after) mean=" << accum.getMean()
                //             << ", sum=" << accum.getSum() << ", count=" << accum.getCount() << std::endl;
                //}
                break;
            case MEAN_SUM_DERIV:
                accum.addSample(statmap.at(stat_name)->getSum());
                break;
            default:
                assert(0);
            }
        }

        // After accumulation, compute and collect the time sequence values, and save the
        // initial state for the next time interval
        void collectSeqData(double cur_time, double time_series_interval, uint comp_count) {
            switch (stat_type) {
            case MEAN:
                time_seq_vec.emplace_back(cur_time, (accum - last_state));
                last_state = accum;
                break;
            case MEAN_SNAP:
                // Debug
                //if (stat_name == "fraction_idle_wl") {
                //   std::cout << "StatSeq::collectSeqData(): time " << cur_time << ":"
                //             << stat_name << ": accum mean=" << accum.getMean()
                //             << ", sum=" << accum.getSum() << ", count=" << accum.getCount() << std::endl;
                //}
                time_seq_vec.emplace_back(cur_time, accum);
                break;
            case MEAN_SUM_DERIV:
            case MEAN_CUM_DERIV:
                {
                    double change_over_time = (accum.getMean() - last_state.getMean())/time_series_interval;
                    time_seq_vec.emplace_back(cur_time, change_over_time);
                    last_state = accum;
                    break;
                }
            case MEAN_ACTIVE_PER_COMP:
                {
                    double change_over_time = (accum.getMean() - last_state.getMean())/time_series_interval;
                    double count_per_comp = ((double)accum.getCount())/comp_count;
                    time_seq_vec.emplace_back(cur_time, change_over_time * count_per_comp);
                    last_state = accum;
                    break;
                }
            default:
                assert(0);
            }
        }

        // Output the time sequence
        void printSeq() {
            std::cout << "\n";
            switch (stat_type) {
            case MEAN:
                outputStatSequence_MeanStdDev(time_seq_vec, tag, descrip);
                break;
            case MEAN_SNAP:
                outputStatSequence_MeanStdDevMinMax(time_seq_vec, tag, descrip);
                break;
            case MEAN_SUM_DERIV:
                outputStatSequence_Sum(time_seq_vec, tag, descrip);
                break;
            case MEAN_CUM_DERIV:
            case MEAN_ACTIVE_PER_COMP:
                outputStatSequence_Mean(time_seq_vec, tag, descrip);
                break;
            default:
                assert(0);
            }
        }
    };

    std::vector<StatSeq> stat_seq_vec_;
    uint comp_count_ {0};

public:
    // Add a stat for which we want to generate the time sequence
    void addStat(const std::string &stat_name, StatType stat_type,
                     const std::string &tag, const std::string &descrip)
    {
        stat_seq_vec_.emplace_back(stat_name, stat_type, tag, descrip);
    }

    // Reset all temporary variables prior to accumulation for this time point
    void resetTemp() {
        comp_count_ = 0;
        
        // Reset any temporary variables used in the StatSeqs
        for (auto &stat_seq : stat_seq_vec_) { stat_seq.resetTemp(); }
    }

    // Accumulate stats at the current time (for this component)
    void accumulate(simian::Component &comp) {
        ++comp_count_;
        
        // Update any "snapshot" stats
        comp.computeStatSnapshot();
        
        // Accumulate the data from this Component for each of the stat sequences
        const std::map<std::string, simian::StatSampler*> &statmap = comp.getStats();
        for (auto &stat_seq : stat_seq_vec_) { stat_seq.accumulate(statmap); }
    }

    // After accumulation, compute and collect the time sequence values, and save the
    // initial state for the next time interval
    void collectSeqData(double cur_time, double time_series_interval) {
        for (auto &stat_seq : stat_seq_vec_) {
            stat_seq.collectSeqData(cur_time, time_series_interval, comp_count_);
        }
    }

    // Print all of the time sequences
    void printSeqs() {
        for (auto &stat_seq : stat_seq_vec_) { stat_seq.printSeq(); }
    }
};


#endif // __STAT_UTILS_HRR_HPP__
