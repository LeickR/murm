// MIT License
//
// Copyright (c) 2026 Leick Robinson (Virtual Science Workshop)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// 
// StatSampler
// 

#ifndef __MURM_STAT_HPP__
#define __MURM_STAT_HPP__

#include <string>
#include <sstream>
#include <cassert>
#include <cstdint>
#include <cfloat>
#include <cmath>
#include "Component.hpp"

namespace murm {

class StatSampler
{
private:
    Component *owner_{nullptr};
    std::string name_;
    std::string full_name_;
    std::string descrip_;

    uint64_t count_{0};
    double sum_{0.0};
    double sum_squares_{0.0};
    double min_{DBL_MAX};
    double max_{0.0};
    std::string min_label_;
    std::string max_label_;
    double min_time_{0.0};
    double max_time_{0.0};

    bool is_accumulator_{false}; // If true, only accumulate the sum

public:
    
    static const bool IS_ACCUMULATOR = true;
    
    // Constructors
    StatSampler()
    {}

    StatSampler(Component *owner, const std::string &name, const std::string &descrip,
                    bool is_accumulator = false) :
        owner_(owner),
        name_(name),
        descrip_(descrip),
        is_accumulator_(is_accumulator)
    {
        // StatSamplers follow the same name validity rules as Components
        //assert(Component::isValidName(name_));

        if (owner_) {
            // Prepend the owner's full name to get the StatSampler's full name
            full_name_ = owner_->getFullName() + '.' + name_;
            
            // Add this to the owner's stat list (to enable global access)
            owner_->addStatSampler(this, name);
        }
    }

    // Convenience constructor for creating a single-sample StatSampler
    StatSampler(double sample)
    {
        addSample(sample);
    }

    // Copy constructor
    // (we don't need to copy the name, description, or owner pointer)
    StatSampler(const StatSampler &orig) :
        count_(orig.count_),
        sum_(orig.sum_),
        sum_squares_(orig.sum_squares_),
        min_(orig.min_),
        max_(orig.max_),
        min_label_(orig.min_label_),
        max_label_(orig.max_label_),
        min_time_(orig.min_time_),
        max_time_(orig.max_time_),
        is_accumulator_(orig.is_accumulator_)
    {}

    // = operator
    // (we don't need to copy the name, description, or owner pointer)
    const StatSampler &operator=(const StatSampler &rhs) {
        count_ = rhs.count_;
        sum_ = rhs.sum_;
        sum_squares_ = rhs.sum_squares_;
        min_ = rhs.min_;
        max_ = rhs.max_;
        min_label_ = rhs.min_label_;
        max_label_ = rhs.max_label_;
        min_time_ = rhs.min_time_;
        max_time_ = rhs.max_time_;
        is_accumulator_ = rhs.is_accumulator_;

        return (*this);
    }

    // Add two StatSamplers
    const StatSampler operator+(const StatSampler &rhs) const
    {
        StatSampler result;
        result.count_ = count_ + rhs.count_;
        result.sum_ = sum_ + rhs.sum_;
        result.is_accumulator_ = (is_accumulator_ || rhs.is_accumulator_);
        if (result.is_accumulator_ == false) {
            result.sum_squares_ = sum_squares_ + rhs.sum_squares_;
            if (rhs.min_ < min_) {
                result.min_ = rhs.min_;
                result.min_label_ = rhs.min_label_;
                result.min_time_ = rhs.min_time_;
            } else {
                result.min_ = min_;
                result.min_label_ = min_label_;
                result.min_time_ = min_time_;
            }
            if (rhs.max_ > max_) {
                result.max_ = rhs.max_;
                result.max_label_ = rhs.max_label_;
                result.max_time_ = rhs.max_time_;
            } else {
                result.max_ = max_;
                result.max_label_ = max_label_;
                result.max_time_ = max_time_;
            }
        }

        return result;
    }

    // Add another StatSampler to this one
    const StatSampler &operator+=(const StatSampler &rhs) {
        count_ += rhs.count_;
        sum_ += rhs.sum_;
        is_accumulator_ = (is_accumulator_ || rhs.is_accumulator_);
        if (is_accumulator_ == false) {
            sum_squares_ += rhs.sum_squares_;
            if (rhs.min_ < min_) {
                min_ = rhs.min_;
                min_label_ = rhs.min_label_;
                min_time_ = rhs.min_time_;
            }
            if (rhs.max_ > max_) {
                max_ = rhs.max_;
                max_label_ = rhs.max_label_;
                max_time_ = rhs.max_time_;
            }
        }
        
        return (*this);
    }

    // Subtract two StatSamplers
    const StatSampler operator-(const StatSampler &rhs) const
    {
        StatSampler result;
        result.count_ = count_ - rhs.count_;
        result.sum_ = sum_ - rhs.sum_;
        result.is_accumulator_ = (is_accumulator_ || rhs.is_accumulator_);
        if (result.is_accumulator_ == false) {
            result.sum_squares_ = sum_squares_ - rhs.sum_squares_;
            // min_ and max_ are invalidated on subtraction
        }

        return result;
    }

    // Add a sample
    void addSample(double sample) {
        assert(std::isnan(sample) == false);

        ++count_;
        sum_ += sample;
        if (is_accumulator_ == false) {
            sum_squares_ += sample * sample;
            if (sample < min_) {
                min_ = sample;
                min_label_ = full_name_;
                if (owner_) { min_time_ = owner_->getTime(); }
            }
            if (sample > max_) {
                max_ = sample;
                max_label_ = full_name_;
                if (owner_) { max_time_ = owner_->getTime(); }
            }
        }
    }

    // Accumulate (same as addSample(), but this syntax is typically used when this is just
    // an accumulator)
    const StatSampler &operator+=(double sample) {
        addSample(sample);
        return (*this);
    }

    // Increment an accumulator by one (effectively, adds a sample of value 1).
    // (only really makes sense if this is just an accumulator)
    // (prefix increment)
    StatSampler &operator++() {
        addSample(1.0);
        return (*this);
    }

    // (postfix increment) [Try not to use this!!]
    StatSampler operator++(int) {
        StatSampler tmp(*this);
        operator++();
        return tmp;
    }

    // Reset the StatSampler
    void reset() {
        count_ = 0;
        sum_ = 0.0;
        sum_squares_ = 0.0;
        min_ = DBL_MAX;
        max_ = 0.0;
        min_label_.clear();
        max_label_.clear();
        min_time_ = 0.0;
        max_time_ = 0.0;
    }

    // Accessors
    const std::string &getName() const { return name_; }
    const std::string &getFullName() const { return full_name_; }
    const std::string &getDescrip() const { return descrip_; }

    bool isAccumulator() const { return is_accumulator_; }

    uint64_t getCount() const { return count_; }
    double getSum() const { return sum_; }

    // Computed values
    double getMean() const { return (count_ > 0) ? (sum_/count_) : (0.0); }
    double getStdDev() const {
        double mean = getMean();
        return (count_ > 0) ? (std::sqrt((sum_squares_/count_) - (mean*mean))) : (0.0);
    }

    // Min/max
    bool isMinValid() const { return (min_ != DBL_MAX); }
    double getMin() const { return min_; }
    const std::string &getMinLabel() const { return min_label_; }
    double getMinTime() const { return min_time_; }

    bool isMaxValid() const { return (max_ != 0.0) || ((sum_ == 0.0) && (count_ != 0)); }
    double getMax() const { return max_; }
    const std::string &getMaxLabel() const { return max_label_; }
    double getMaxTime() const { return max_time_; }
};


// StatSampler time sequence
typedef std::vector<std::pair<double, murm::StatSampler>> StatSamplerTimeSequenceVec;


} // namespace murm

#endif // __MURM_STAT_HPP__
