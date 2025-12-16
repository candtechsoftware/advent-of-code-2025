#pragma once
#include "base.h"
#include "string_core.h"
#include "arena.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
} Log_Level;

typedef u32 Log_Flags;
enum {
    Log_Flag_NoTimestamp = (1 << 0), // Skip timestamp for speed
    Log_Flag_NoLocation = (1 << 1),  // Skip file:line for speed
};

#define LOG_BUFFER_SIZE KB(64)

typedef struct Logger_State Logger_State;
struct Logger_State {
    Arena *arena;
    FILE  *log_file;
    b32    initialized;

    u8 *buffer;
    u64 buffer_pos;
    u64 buffer_cap;

    Log_Flags flags;

    char cached_timestamp[32];
    u64  cached_timestamp_len;
    b32  timestamp_valid;
#if OS_LINUX || OS_MAC
    int file_fd; // Raw file descriptor for unbuffered writes
#elif OS_WINDOWS
    void *file_handle; // HANDLE for unbuffered writes
#endif
};

void log_init(Arena *arena, String log_file_path);
void log_shutdown(void);
void log_flush(void);
void log_set_flags(Log_Flags flags);
void log_update_timestamp(void);

void log_impl(Log_Level level, String file, u32 line, char *fmt, ...);

#if DEBUG_MODE
#    define log_debug(fmt, ...) log_impl(LOG_LEVEL_DEBUG, str_lit(__FILE__), __LINE__, fmt, ##__VA_ARGS__)
#else
#    define log_debug(fmt, ...) ((void)0)
#endif

#define log_info(fmt, ...)  log_impl(LOG_LEVEL_INFO, str_lit(__FILE__), __LINE__, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  log_impl(LOG_LEVEL_WARN, str_lit(__FILE__), __LINE__, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log_impl(LOG_LEVEL_ERROR, str_lit(__FILE__), __LINE__, fmt, ##__VA_ARGS__)

void log_print(const char *fmt, ...);
