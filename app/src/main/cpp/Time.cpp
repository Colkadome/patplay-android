//
// Created by joe on 11/11/2023.
//

#include "Time.h"

#include <chrono>

using Clock = std::chrono::system_clock;
using Seconds = std::chrono::duration<double>;
using TimePoint = std::chrono::time_point<Clock, Seconds>;

float Time::get_dt() {

    const TimePoint now = Clock::now();
    const Seconds duration = now - last_time_;
    last_time_ = now;

    auto count = (float) duration.count();
    return count > 0.02 ? 0.02 : count;
}
