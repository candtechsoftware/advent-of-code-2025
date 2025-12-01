#include <time.h>
#include <string.h>

#if OS_LINUX || OS_MAC
#    include <unistd.h>
#    include <fcntl.h>
#endif

static Logger_State *g_logger_state = NULL;

#if OS_LINUX || OS_MAC
static void
log_write_raw(const u8 *data, u64 size) {
    if (g_logger_state->file_fd >= 0) {
        write(g_logger_state->file_fd, data, size);
    } else {
        write(STDOUT_FILENO, data, size);
    }
}
#elif OS_WINDOWS
#    include <windows.h>
static void
log_write_raw(const u8 *data, u64 size) {
    DWORD written;
    if (g_logger_state->file_handle) {
        WriteFile(g_logger_state->file_handle, data, (DWORD)size, &written, NULL);
    } else {
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), data, (DWORD)size, &written, NULL);
    }
}
#else
static void
log_write_raw(const u8 *data, u64 size) {
    fwrite(data, 1, size, stdout);
    fflush(stdout);
}
#endif

static void
log_flush_buffer(void) {
    if (!g_logger_state || g_logger_state->buffer_pos == 0) {
        return;
    }
    log_write_raw(g_logger_state->buffer, g_logger_state->buffer_pos);
    g_logger_state->buffer_pos = 0;
}

static void
log_buffer_write(const u8 *data, u64 size) {
    if (!g_logger_state) return;

    if (size > g_logger_state->buffer_cap) {
        log_flush_buffer();
        log_write_raw(data, size);
        return;
    }

    if (g_logger_state->buffer_pos + size > g_logger_state->buffer_cap) {
        log_flush_buffer();
    }

    MemoryCopy(g_logger_state->buffer + g_logger_state->buffer_pos, data, size);
    g_logger_state->buffer_pos += size;
}

void
log_update_timestamp(void) {
    if (!g_logger_state) return;

    time_t    t = time(NULL);
    struct tm tm = {0};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    g_logger_state->cached_timestamp_len = (u64)snprintf(
        g_logger_state->cached_timestamp,
        sizeof(g_logger_state->cached_timestamp),
        "[%04d-%02d-%02d %02d:%02d:%02d]",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    g_logger_state->timestamp_valid = 1;
}

static const char *log_level_strings[] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
};

static String
log_level_str(Log_Level level) {
    if (level < 0 || level > LOG_LEVEL_ERROR) {
        return str_lit("?????");
    }
    return str((char *)log_level_strings[level], 5);
}

static String
get_filename_from_path(String file_path) {
    String result = file_path;
    for (u64 i = file_path.size; i > 0; i--) {
        if (file_path.str[i - 1] == '/' || file_path.str[i - 1] == '\\') {
            result.str = file_path.str + i;
            result.size = file_path.size - i;
            break;
        }
    }
    return result;
}

void
log_init(Arena *arena, String log_file_path) {
    if (g_logger_state) {
        return;
    }

    g_logger_state = push_array(arena, Logger_State, 1);
    MemoryZeroStruct(g_logger_state);

    g_logger_state->arena = arena;
    g_logger_state->initialized = 1;
    g_logger_state->flags = Log_Flag_Buffered;

    g_logger_state->buffer_cap = LOG_BUFFER_SIZE;
    g_logger_state->buffer = push_array(arena, u8, LOG_BUFFER_SIZE);
    g_logger_state->buffer_pos = 0;

#if OS_LINUX || OS_MAC
    g_logger_state->file_fd = -1;
#elif OS_WINDOWS
    g_logger_state->file_handle = NULL;
#endif

    log_update_timestamp();

#if !DEBUG_MODE
    Scratch temp = arena_begin_scratch(arena);
    String  actual_log_path;

    if (log_file_path.size > 0) {
        actual_log_path = log_file_path;
    } else {
        time_t    t = time(NULL);
        struct tm tm = {0};
#    ifdef _WIN32
        localtime_s(&tm, &t);
#    else
        localtime_r(&t, &tm);
#    endif
        actual_log_path = str_fmt(temp.arena, "build/profile_{d}_{d}_{d}.log",
                                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    }

    String null_term_path = str_fmt(temp.arena, "{S}", actual_log_path);

#    if OS_LINUX || OS_MAC
    g_logger_state->file_fd = open((char *)null_term_path.str,
                                    O_WRONLY | O_CREAT | O_APPEND,
                                    0644);
    if (g_logger_state->file_fd < 0) {
        log_buffer_write((u8 *)"Logger: Failed to open log file\n", 33);
    }
#    elif OS_WINDOWS
    g_logger_state->file_handle = CreateFileA(
        (char *)null_term_path.str,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (g_logger_state->file_handle != INVALID_HANDLE_VALUE) {
        SetFilePointer(g_logger_state->file_handle, 0, NULL, FILE_END);
    } else {
        g_logger_state->file_handle = NULL;
    }
#    else
#        ifdef _WIN32
    fopen_s(&g_logger_state->log_file, (char *)null_term_path.str, "a");
#        else
    g_logger_state->log_file = fopen((char *)null_term_path.str, "a");
#        endif
#    endif

    arena_end_scratch(&temp);
#endif
}

void
log_shutdown(void) {
    if (!g_logger_state) {
        return;
    }

    log_flush_buffer();

#if OS_LINUX || OS_MAC
    if (g_logger_state->file_fd >= 0) {
        close(g_logger_state->file_fd);
        g_logger_state->file_fd = -1;
    }
#elif OS_WINDOWS
    if (g_logger_state->file_handle) {
        CloseHandle(g_logger_state->file_handle);
        g_logger_state->file_handle = NULL;
    }
#endif

    if (g_logger_state->log_file) {
        fclose(g_logger_state->log_file);
        g_logger_state->log_file = NULL;
    }

    g_logger_state = NULL;
}

void
log_flush(void) {
    log_flush_buffer();
}

void
log_set_flags(Log_Flags flags) {
    if (g_logger_state) {
        g_logger_state->flags = flags;
    }
}

void
log_impl(Log_Level level, String file, u32 line, char *fmt, ...) {
    if (!g_logger_state || !g_logger_state->initialized) return;

    Scratch temp = arena_begin_scratch(g_logger_state->arena);

    va_list args;
    va_start(args, fmt);
    String user_msg = str_fmtv(temp.arena, fmt, args);
    va_end(args);

    String log_msg;

    if (g_logger_state->flags & Log_Flag_NoTimestamp) {
        if (g_logger_state->flags & Log_Flag_NoLocation) {
            String level_str = log_level_str(level);
            log_msg = str_fmt(temp.arena, "[{S}] {S}\n", level_str, user_msg);
        } else {
            String filename = get_filename_from_path(file);
            String level_str = log_level_str(level);
            log_msg = str_fmt(temp.arena, "[{S}] {S}:{d} - {S}\n",
                              level_str, filename, line, user_msg);
        }
    } else {
        if (!g_logger_state->timestamp_valid) log_update_timestamp();

        String timestamp = str((char *)g_logger_state->cached_timestamp,
                               g_logger_state->cached_timestamp_len);
        String level_str = log_level_str(level);

        if (g_logger_state->flags & Log_Flag_NoLocation) {
            log_msg = str_fmt(temp.arena, "{S} [{S}] {S}\n",
                              timestamp, level_str, user_msg);
        } else {
            String filename = get_filename_from_path(file);
            log_msg = str_fmt(temp.arena, "{S} [{S}] {S}:{d} - {S}\n",
                              timestamp, level_str, filename, line, user_msg);
        }
    }

    if (g_logger_state->flags & Log_Flag_Buffered) {
        log_buffer_write(log_msg.str, log_msg.size);
        if (g_logger_state->flags & Log_Flag_FlushNow) log_flush_buffer();
    } else {
        log_write_raw(log_msg.str, log_msg.size);
    }

    arena_end_scratch(&temp);
}

void
log_print(const char *fmt, ...) {
    if (!g_logger_state || !g_logger_state->initialized) return;

    Scratch temp = arena_begin_scratch(g_logger_state->arena);

    va_list args;
    va_start(args, fmt);
    String msg = str_fmtv(temp.arena, fmt, args);
    va_end(args);

    if (g_logger_state->flags & Log_Flag_Buffered) {
        log_buffer_write(msg.str, msg.size);
    } else {
        log_write_raw(msg.str, msg.size);
    }

    arena_end_scratch(&temp);
}
