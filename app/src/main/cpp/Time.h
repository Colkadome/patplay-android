//
// Created by joe on 11/11/2023.
//

#ifndef PAT_PLAY_TIME_H
#define PAT_PLAY_TIME_H

#include <chrono>

using Clock = std::chrono::system_clock;
using Seconds = std::chrono::duration<double>;
using TimePoint = std::chrono::time_point<Clock, Seconds>;

class Time {
public:

    inline Time(): last_time_(Clock::now()) {}

    /*!
     * Gets the elapsed time since last call.
     * @return elapsed time.
     */
    float get_dt();

private:
    TimePoint last_time_;

};

#endif //PAT_PLAY_TIME_H
