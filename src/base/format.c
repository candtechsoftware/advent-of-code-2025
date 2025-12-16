#include <string.h>

static const char fmt_digits_lower[] = "0123456789abcdef";
static const char fmt_digits_upper[] = "0123456789ABCDEF";

static u32
fmt_u64_to_str(u64 val, char *buf, u32 base) {
    char tmp[64];
    u32  i = 0;

    if (val == 0) {
        buf[0] = '0';
        return 1;
    }

    const char *digits = fmt_digits_lower;
    while (val) {
        tmp[i++] = digits[val % base];
        val /= base;
    }

    u32 len = i;
    while (i--) {
        *buf++ = tmp[i];
    }
    return len;
}

static u32
fmt_s64_to_str(s64 val, char *buf) {
    u32 len = 0;

    if (val < 0) {
        buf[0] = '-';
        len = 1;
        val = -val;
    }

    len += fmt_u64_to_str((u64)val, buf + len, 10);
    return len;
}

static u32
fmt_f64_to_str(f64 val, char *buf, u32 precision) {
    u32 len = 0;

    if (val < 0) {
        buf[len++] = '-';
        val = -val;
    }

    if (val != val) {
        buf[len++] = 'n';
        buf[len++] = 'a';
        buf[len++] = 'n';
        return len;
    }

    u64 int_part = (u64)val;
    len += fmt_u64_to_str(int_part, buf + len, 10);
    buf[len++] = '.';

    f64 frac = val - (f64)int_part;
    if (precision == 0) precision = 6;

    for (u32 i = 0; i < precision; i++) {
        frac *= 10.0;
        u32 digit = (u32)frac;
        buf[len++] = '0' + digit;
        frac -= digit;
    }

    return len;
}

static u32
fmt_u64_to_hex(u64 val, char *buf, b32 uppercase) {
    char tmp[16];
    u32  i = 0;

    if (val == 0) {
        buf[0] = '0';
        return 1;
    }

    const char *digits = uppercase ? fmt_digits_upper : fmt_digits_lower;
    while (val) {
        tmp[i++] = digits[val & 0xF];
        val >>= 4;
    }

    u32 len = i;
    while (i--) {
        *buf++ = tmp[i];
    }
    return len;
}

static u32
fmt_u64_to_bin(u64 val, char *buf) {
    char tmp[64];
    u32  i = 0;

    if (val == 0) {
        buf[0] = '0';
        return 1;
    }

    while (val) {
        tmp[i++] = '0' + (val & 1);
        val >>= 1;
    }

    u32 len = i;
    while (i--) {
        *buf++ = tmp[i];
    }
    return len;
}

//
// Core formatter - callback-based design
//

typedef void (*Fmt_Output_Func)(void *ctx, const char *data, u64 len);

static u64
str_fmt_core(Fmt_Output_Func output, void *ctx, const char *fmt, va_list args) {
    char num_buf[128];
    u64  total_len = 0;
    const char *p = fmt;
    const char *chunk_start = p;

    while (*p) {
        if (*p == '{') {
            if (p > chunk_start) {
                u64 chunk_len = (u64)(p - chunk_start);
                if (output) output(ctx, chunk_start, chunk_len);
                total_len += chunk_len;
            }

            if (*(p + 1) == '{') {
                if (output) output(ctx, "{", 1);
                total_len += 1;
                p += 2;
                chunk_start = p;
                continue;
            }

            const char *spec_start = p + 1;
            while (*spec_start && *spec_start != '}') spec_start++;

            if (*spec_start == '}') {
                char spec = *(p + 1);

                switch (spec) {
                    case 's': {
                        const char *str = va_arg(args, const char *);
                        if (str) {
                            u64 len = strlen(str);
                            if (output) output(ctx, str, len);
                            total_len += len;
                        } else {
                            if (output) output(ctx, "(null)", 6);
                            total_len += 6;
                        }
                        break;
                    }
                    case 'S': {
                        String str = va_arg(args, String);
                        if (str.str && str.size > 0) {
                            if (output) output(ctx, (const char *)str.str, str.size);
                            total_len += str.size;
                        }
                        break;
                    }
                    case 'd': {
                        s64 val = va_arg(args, int);
                        u32 len = fmt_s64_to_str(val, num_buf);
                        if (output) output(ctx, num_buf, len);
                        total_len += len;
                        break;
                    }
                    case 'u': {
                        u64 val = va_arg(args, unsigned int);
                        u32 len = fmt_u64_to_str(val, num_buf, 10);
                        if (output) output(ctx, num_buf, len);
                        total_len += len;
                        break;
                    }
                    case 'x': {
                        u64 val = va_arg(args, unsigned int);
                        u32 len = fmt_u64_to_hex(val, num_buf, 0);
                        if (output) output(ctx, num_buf, len);
                        total_len += len;
                        break;
                    }
                    case 'X': {
                        u64 val = va_arg(args, unsigned int);
                        u32 len = fmt_u64_to_hex(val, num_buf, 1);
                        if (output) output(ctx, num_buf, len);
                        total_len += len;
                        break;
                    }
                    case 'b': {
                        u64 val = va_arg(args, unsigned int);
                        u32 len = fmt_u64_to_bin(val, num_buf);
                        if (output) output(ctx, num_buf, len);
                        total_len += len;
                        break;
                    }
                    case 'f': {
                        f64 val = va_arg(args, double);
                        u32 len = fmt_f64_to_str(val, num_buf, 6);
                        if (output) output(ctx, num_buf, len);
                        total_len += len;
                        break;
                    }
                    case 'p': {
                        void *ptr = va_arg(args, void *);
                        if (output) output(ctx, "0x", 2);
                        total_len += 2;
                        u32 len = fmt_u64_to_hex((u64)(uintptr_t)ptr, num_buf, 0);
                        if (output) output(ctx, num_buf, len);
                        total_len += len;
                        break;
                    }
                    case 'o': {
                        va_arg(args, void *);
                        if (output) output(ctx, "{ object }", 10);
                        total_len += 10;
                        break;
                    }
                    default: {
                        if (output) {
                            output(ctx, "{", 1);
                            output(ctx, &spec, 1);
                            output(ctx, "}", 1);
                        }
                        total_len += 3;
                        break;
                    }
                }

                p += 3;
                chunk_start = p;
                continue;
            }
        }

        if (*p == '}' && *(p + 1) == '}') {
            if (p > chunk_start) {
                u64 chunk_len = (u64)(p - chunk_start);
                if (output) output(ctx, chunk_start, chunk_len);
                total_len += chunk_len;
            }
            if (output) output(ctx, "}", 1);
            total_len += 1;
            p += 2;
            chunk_start = p;
            continue;
        }

        p++;
    }

    if (p > chunk_start) {
        u64 chunk_len = (u64)(p - chunk_start);
        if (output) output(ctx, chunk_start, chunk_len);
        total_len += chunk_len;
    }

    return total_len;
}

//
// Arena output
//

typedef struct {
    Arena *arena;
    u8    *buf;
    u64    buf_cap;
    u64    buf_pos;
} Fmt_Arena_Ctx;

static void
fmt_arena_output(void *ctx, const char *data, u64 len) {
    Fmt_Arena_Ctx *c = (Fmt_Arena_Ctx *)ctx;

    while (c->buf_pos + len >= c->buf_cap) {
        u64  new_cap = c->buf_cap * 2;
        u8  *new_buf = push_array(c->arena, u8, new_cap);
        MemoryCopy(new_buf, c->buf, c->buf_pos);
        c->buf = new_buf;
        c->buf_cap = new_cap;
    }

    MemoryCopy(c->buf + c->buf_pos, data, len);
    c->buf_pos += len;
}

static String
str_fmtv(Arena *arena, const char *fmt, va_list args) {
    u64 fmt_len = strlen(fmt);
    u64 buf_cap = (fmt_len < 128) ? 256 : fmt_len * 2;

    Fmt_Arena_Ctx ctx = {
        .arena   = arena,
        .buf     = push_array(arena, u8, buf_cap),
        .buf_cap = buf_cap,
        .buf_pos = 0,
    };

    str_fmt_core(fmt_arena_output, &ctx, fmt, args);

    ctx.buf[ctx.buf_pos] = '\0';
    return (String){ctx.buf, ctx.buf_pos};
}

static String
str_fmt(Arena *arena, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String result = str_fmtv(arena, fmt, args);
    va_end(args);
    return result;
}

//
// Buffer output
//

typedef struct {
    char *buf;
    int   count;
    int   pos;
} Fmt_Buffer_Ctx;

static void
fmt_buffer_output(void *ctx, const char *data, u64 len) {
    Fmt_Buffer_Ctx *c = (Fmt_Buffer_Ctx *)ctx;

    if (c->buf && c->pos < c->count) {
        int avail = c->count - c->pos - 1;
        int to_copy = (int)len;
        if (to_copy > avail) to_copy = avail;
        if (to_copy > 0) {
            MemoryCopy(c->buf + c->pos, data, to_copy);
        }
    }

    c->pos += (int)len;
}

static int
str_vsnprintf(char *buf, int count, const char *fmt, va_list args) {
    if (count == 0 && buf == 0) {
        va_list args_copy;
        va_copy(args_copy, args);
        u64 len = str_fmt_core(0, 0, fmt, args_copy);
        va_end(args_copy);
        return (int)len;
    }

    Fmt_Buffer_Ctx ctx = {
        .buf   = buf,
        .count = count,
        .pos   = 0,
    };

    str_fmt_core(fmt_buffer_output, &ctx, fmt, args);

    if (buf && count > 0) {
        int term_pos = ctx.pos;
        if (term_pos >= count) term_pos = count - 1;
        buf[term_pos] = '\0';
    }

    return ctx.pos;
}

static int
str_snprintf(char *buf, int count, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = str_vsnprintf(buf, count, fmt, args);
    va_end(args);
    return result;
}

//
// Stdout output
//

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
static void
print_raw(const char *data, u64 len) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(h, data, (DWORD)len, &written, NULL);
    }
}
#else
#include <unistd.h>
static void
print_raw(const char *data, u64 len) {
    write(STDOUT_FILENO, data, len);
}
#endif

static void
fmt_stdout_output(void *ctx, const char *data, u64 len) {
    (void)ctx;
    print_raw(data, len);
}

static void
printv(const char *fmt, va_list args) {
    str_fmt_core(fmt_stdout_output, 0, fmt, args);
}

static void
print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printv(fmt, args);
    va_end(args);
}
