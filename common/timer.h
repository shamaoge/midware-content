
#pragma once

#include <chrono>
#include <sys/time.h>

using namespace std::chrono; 

#define TIME_SINCE_EPOCH(tv) \
        int64_t tv = std::chrono::duration_cast<std::chrono::milliseconds>(         \
                    std::chrono::system_clock::now().time_since_epoch()             \
                    ).count();

#define TIMER_NOW(tv)                                                                   \
        tv = std::chrono::steady_clock::now();

#define TIMER_ELAPSE(start, end, elapse)                                                \
        do {                                                                            \
            std::chrono::microseconds __elapse__                                        \
                = std::chrono::duration_cast<std::chrono::microseconds>(end-start);     \
            elapse = __elapse__.count()/1000.0;                                         \
        } while(0);

#define TIMER(tv)                                                                       \
    std::chrono::steady_clock::time_point tv = std::chrono::steady_clock::now();

#define TIMER_END(tv, elapse)                                                           \
        double elapse = 0;                                                              \
        do {                                                                            \
            std::chrono::steady_clock::time_point _tv_end_                              \
                = std::chrono::steady_clock::now();                                     \
            std::chrono::microseconds __elapse__ =                                      \
                = std::chrono::duration_cast<std::chrono::microseconds>(_tv_end_-tv);   \
            elapse = __elapse__.count()/1000.0;                                         \
        } while(0);

#define CTIMER(tv)                                                                      \
        struct timeval tv;                                                              \
        gettimeofday(&tv, NULL);

#define CTIMER_END(tv, elapse)                                                          \
        double elapse = 0;                                                              \
        do {                                                                            \
            TIMER(__timer_end);                                                         \
            elapse = ((__timer_end.tv_sec*1000000L + __timer_end.tv_usec)               \
                        - (tv.tv_sec*1000000L + tv.tv_usec))/1000.0;                    \
        } while(0);

