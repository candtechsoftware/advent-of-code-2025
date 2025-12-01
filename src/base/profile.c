#if PROFILE_MODE

#    include <stdio.h>
#    include <string.h>

#    if OS_MAC
#        include <sys/time.h>
#        include <mach/mach_time.h>
#        include <sys/mman.h>
#        include <unistd.h>
#        include <pthread.h>
#    endif

#    if OS_WINDOWS
#        include <windows.h>
#    endif

#    if OS_LINUX
#        include <time.h>
#        include <sys/mman.h>
#        include <unistd.h>
#        include <pthread.h>
#        include <sys/syscall.h>
#    endif

static FILE  *prof_file = NULL;
static b32    prof_first_event = 1;
static u64    prof_start_time = 0;
static double prof_ticks_to_us = 1.0;

static THREAD_VAR b32 prof_thread_initialized = 0;
static THREAD_VAR u32 prof_tid = 0;

#    if OS_MAC

static double prof_get_ticks_to_us(void) {
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    return (double)timebase.numer / (double)timebase.denom / 1000.0;
}

static u64 prof_rdtsc(void) {
    return mach_absolute_time();
}

static u32 prof_get_tid(void) {
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return (u32)tid;
}

#    elif OS_WINDOWS

static double prof_get_ticks_to_us(void) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return 1000000.0 / (double)freq.QuadPart;
}

static u64 prof_rdtsc(void) {
    return __rdtsc();
}

static u32 prof_get_tid(void) {
    return (u32)GetCurrentThreadId();
}

#    elif OS_LINUX

static double prof_get_ticks_to_us(void) {
#        if ARCH_X64
    struct timespec start_ts, end_ts;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    u64 start_tsc = ({
        u32 lo, hi;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        ((u64)hi << 32) | lo;
    });

    struct timespec delay = {0, 10000000};
    nanosleep(&delay, NULL);

    clock_gettime(CLOCK_MONOTONIC, &end_ts);
    u64 end_tsc = ({
        u32 lo, hi;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        ((u64)hi << 32) | lo;
    });

    u64 elapsed_ns = (end_ts.tv_sec - start_ts.tv_sec) * 1000000000ULL +
                     (end_ts.tv_nsec - start_ts.tv_nsec);
    u64 elapsed_tsc = end_tsc - start_tsc;

    return (double)elapsed_ns / 1000.0 / (double)elapsed_tsc;
#        else
    return 0.001;
#        endif
}

static u64 prof_rdtsc(void) {
#        if ARCH_X64
    u32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
#        else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
#        endif
}

static u32 prof_get_tid(void) {
    return (u32)syscall(SYS_gettid);
}

#    else
#        error Need profiling implementation for this OS
#    endif

static u32 prof_get_pid(void) {
#    if OS_WINDOWS
    return (u32)GetCurrentProcessId();
#    else
    return (u32)getpid();
#    endif
}

void prof_open(char *name) {
    prof_file = fopen(name, "w");
    if (!prof_file)
        return;

    prof_ticks_to_us = prof_get_ticks_to_us();
    prof_start_time = prof_rdtsc();
    prof_first_event = 1;

    fprintf(prof_file, "{\"traceEvents\":[\n");
}

void prof_close(void) {
    if (!prof_file)
        return;

    fprintf(prof_file, "\n]}\n");
    fclose(prof_file);
    prof_file = NULL;
}

void prof_thread_begin(void) {
    prof_thread_initialized = 1;
    prof_tid = prof_get_tid();
}

void prof_thread_end(void) {
    prof_thread_initialized = 0;
}

void prof_thread_flush(void) {
    if (prof_file) {
        fflush(prof_file);
    }
}

void prof_begin(const u8 *name, u32 len) {
    if (!prof_file || !prof_thread_initialized)
        return;

    u64    now = prof_rdtsc();
    double ts_us = (double)(now - prof_start_time) * prof_ticks_to_us;
    u32    pid = prof_get_pid();

    if (!prof_first_event) {
        fprintf(prof_file, ",\n");
    }
    prof_first_event = 0;

    fprintf(prof_file, "{\"ph\":\"B\",\"name\":\"%.*s\",\"ts\":%.3f,\"pid\":%u,\"tid\":%u}",
            len, name, ts_us, pid, prof_tid);
}

void prof_end(void) {
    if (!prof_file || !prof_thread_initialized)
        return;

    u64    now = prof_rdtsc();
    double ts_us = (double)(now - prof_start_time) * prof_ticks_to_us;
    u32    pid = prof_get_pid();

    if (!prof_first_event) {
        fprintf(prof_file, ",\n");
    }
    prof_first_event = 0;

    fprintf(prof_file, "{\"ph\":\"E\",\"ts\":%.3f,\"pid\":%u,\"tid\":%u}",
            ts_us, pid, prof_tid);
}

#endif // PROFILE_MODE
