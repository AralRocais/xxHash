/*
 * Copyright (c) 2019-present, Yann Collet, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */


/* ===  Dependencies  === */

#include "timefn.h"


/*-****************************************
*  Time functions
******************************************/

#if defined(_WIN32)   /* Windows */

UTIL_time_t UTIL_getTime(void) { UTIL_time_t x; QueryPerformanceCounter(&x); return x; }

U64 UTIL_getSpanTimeMicro(UTIL_time_t clockStart, UTIL_time_t clockEnd)
{
    static LARGE_INTEGER ticksPerSecond;
    static int init = 0;
    if (!init) {
        if (!QueryPerformanceFrequency(&ticksPerSecond))
            UTIL_DISPLAYLEVEL(1, "ERROR: QueryPerformanceFrequency() failure\n");
        init = 1;
    }
    return 1000000ULL*(clockEnd.QuadPart - clockStart.QuadPart)/ticksPerSecond.QuadPart;
}

U64 UTIL_getSpanTimeNano(UTIL_time_t clockStart, UTIL_time_t clockEnd)
{
    static LARGE_INTEGER ticksPerSecond;
    static int init = 0;
    if (!init) {
        if (!QueryPerformanceFrequency(&ticksPerSecond))
            UTIL_DISPLAYLEVEL(1, "ERROR: QueryPerformanceFrequency() failure\n");
        init = 1;
    }
    return 1000000000ULL*(clockEnd.QuadPart - clockStart.QuadPart)/ticksPerSecond.QuadPart;
}

#elif defined(__APPLE__) && defined(__MACH__)

UTIL_time_t UTIL_getTime(void) { return mach_absolute_time(); }

PTime UTIL_getSpanTimeMicro(UTIL_time_t clockStart, UTIL_time_t clockEnd)
{
    static mach_timebase_info_data_t rate;
    static int init = 0;
    if (!init) {
        mach_timebase_info(&rate);
        init = 1;
    }
    return (((clockEnd - clockStart) * (PTime)rate.numer) / ((PTime)rate.denom))/1000ULL;
}

PTime UTIL_getSpanTimeNano(UTIL_time_t clockStart, UTIL_time_t clockEnd)
{
    static mach_timebase_info_data_t rate;
    static int init = 0;
    if (!init) {
        mach_timebase_info(&rate);
        init = 1;
    }
    return ((clockEnd - clockStart) * (PTime)rate.numer) / ((PTime)rate.denom);
}

#elif (PLATFORM_POSIX_VERSION >= 200112L) \
   && (defined(__UCLIBC__)                \
      || (defined(__GLIBC__)              \
          && ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 17) \
             || (__GLIBC__ > 2))))

UTIL_time_t UTIL_getTime(void)
{
    UTIL_time_t time;
    if (clock_gettime(CLOCK_MONOTONIC, &time))
        UTIL_DISPLAYLEVEL(1, "ERROR: Failed to get time\n");   /* we could also exit() */
    return time;
}

UTIL_time_t UTIL_getSpanTime(UTIL_time_t begin, UTIL_time_t end)
{
    UTIL_time_t diff;
    if (end.tv_nsec < begin.tv_nsec) {
        diff.tv_sec = (end.tv_sec - 1) - begin.tv_sec;
        diff.tv_nsec = (end.tv_nsec + 1000000000ULL) - begin.tv_nsec;
    } else {
        diff.tv_sec = end.tv_sec - begin.tv_sec;
        diff.tv_nsec = end.tv_nsec - begin.tv_nsec;
    }
    return diff;
}

U64 UTIL_getSpanTimeMicro(UTIL_time_t begin, UTIL_time_t end)
{
    UTIL_time_t const diff = UTIL_getSpanTime(begin, end);
    U64 micro = 0;
    micro += 1000000ULL * diff.tv_sec;
    micro += diff.tv_nsec / 1000ULL;
    return micro;
}

U64 UTIL_getSpanTimeNano(UTIL_time_t begin, UTIL_time_t end)
{
    UTIL_time_t const diff = UTIL_getSpanTime(begin, end);
    U64 nano = 0;
    nano += 1000000000ULL * diff.tv_sec;
    nano += diff.tv_nsec;
    return nano;
}

#else   /* relies on standard C (note : clock_t measurements can be wrong when using multi-threading) */

UTIL_time_t UTIL_getTime(void) { return clock(); }
U64 UTIL_getSpanTimeMicro(UTIL_time_t clockStart, UTIL_time_t clockEnd) { return 1000000ULL * (clockEnd - clockStart) / CLOCKS_PER_SEC; }
U64 UTIL_getSpanTimeNano(UTIL_time_t clockStart, UTIL_time_t clockEnd) { return 1000000000ULL * (clockEnd - clockStart) / CLOCKS_PER_SEC; }

#endif

/* returns time span in microseconds */
PTime UTIL_clockSpanMicro(UTIL_time_t clockStart )
{
    UTIL_time_t const clockEnd = UTIL_getTime();
    return UTIL_getSpanTimeMicro(clockStart, clockEnd);
}

/* returns time span in microseconds */
PTime UTIL_clockSpanNano(UTIL_time_t clockStart )
{
    UTIL_time_t const clockEnd = UTIL_getTime();
    return UTIL_getSpanTimeNano(clockStart, clockEnd);
}

void UTIL_waitForNextTick(void)
{
    UTIL_time_t const clockStart = UTIL_getTime();
    UTIL_time_t clockEnd;
    do {
        clockEnd = UTIL_getTime();
    } while (UTIL_getSpanTimeNano(clockStart, clockEnd) == 0);
}
