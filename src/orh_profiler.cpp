/* orh_profiler.cpp - v0.00 - some macros for timing functions and blocks.

REVISION HISTORY:


@Note: Study Computer Enhance Performance-Aware Programming Series!

*/

#include "orh.h"

#if OS_WINDOWS
#include <intrin.h>
#include <windows.h>

FUNCTION u64 get_os_frequency()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq.QuadPart;
}

FUNCTION u64 read_os_timer()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return time.QuadPart;
}

#else
#include <x86intrin.h>
#include <sys/time.h>

FUNCTION u64 get_os_frequency()
{
    return 1000000;
}

FUNCTION u64 read_os_timer()
{
    struct timeval t;
	gettimeofday(&t, 0);
	u64 time = os->time_frequency*(u64)t.tv_sec + (u64)t.tv_usec;
	return time;
}
#endif

FUNCTION inline u64 read_cpu_timer()
{
#if ARCH_X64 || ARCH_X86
    return __rdtsc();
#else
    ASSERT(!"Not Implemented!");
#endif
}

FUNCTION u64 estimate_cpu_frequency()
{
    //
    // @Todo: Why don't we query the frequency using __cpuid()?
    //
    
    u64 ms_to_wait   = 100;
	u64 os_freq      = get_os_frequency();
    
	u64 cpu_start    = read_cpu_timer();
	u64 os_start     = read_os_timer();
	u64 os_end       = 0;
	u64 os_elapsed   = 0;
	u64 os_wait_time = os_freq * ms_to_wait / 1000;
	while(os_elapsed < os_wait_time) {
		os_end     = read_os_timer();
		os_elapsed = os_end - os_start;
	}
	
	u64 cpu_end     = read_cpu_timer();
	u64 cpu_elapsed = cpu_end - cpu_start;
	
	u64 cpu_freq = 0;
	if(os_elapsed) {
		cpu_freq = os_freq * cpu_elapsed / os_elapsed;
	}
	
	return cpu_freq;
}

struct Timed_Block
{
    const char *label;
    u64 cpu_time_start;
    u64 os_time_start;
    u64 timer_frequency;
    
    Timed_Block(const char *label_, u64 timer_frequency_)
    {
        label           = label_;
        timer_frequency = timer_frequency_;
        cpu_time_start  = read_cpu_timer();
        os_time_start   = read_os_timer();
    }
    
    ~Timed_Block()
    {
        u64 os_elapsed = read_os_timer() - os_time_start;
        debug_print("%s: %.4fms\n", label, 1000.0f*os_elapsed / (f64)timer_frequency);
    }
};

#define TIME_SCOPE(name, frequency) Timed_Block GLUE(__block_, __LINE__)(name, frequency)
#define TIME_FUNCTION(frequency) TIME_SCOPE(__func__, frequency)