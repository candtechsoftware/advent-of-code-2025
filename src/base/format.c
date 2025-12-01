#include <string.h>

static const char fmt_digits_lower[] = "0123456789abcdef";
static const char fmt_digits_upper[] = "0123456789ABCDEF";

u32
fmt_u64_to_str(u64 val, char *buf, u32 base) {
    char tmp[64];
    u32  i = 0;
	
    if (val == 0) {
        buf[0] = '0';
        return 1;
    }
	
    const char *digits = (base == 16) ? fmt_digits_lower : fmt_digits_lower;
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

u32
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

u32
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

String
str_fmtv(Arena *arena, const char *fmt, va_list args) {
    u64 fmt_len = strlen(fmt);
    u64 buf_cap = (fmt_len < 128) ? 256 : fmt_len * 2;
    u8 *buf = push_array(arena, u8, buf_cap);
    u64 buf_pos = 0;
    char num_buf[128];
	
    const char *p = fmt;
    while (*p) {
        if (buf_pos + 128 > buf_cap) {
            u64  new_cap = buf_cap * 2;
            u8  *new_buf = push_array(arena, u8, new_cap);
            MemoryCopy(new_buf, buf, buf_pos);
            buf = new_buf;
            buf_cap = new_cap;
        }
		
        if (*p == '{') {
            if (*(p + 1) == '{') {
                buf[buf_pos++] = '{';
                p += 2;
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
							while (buf_pos + len >= buf_cap) {
								u64  new_cap = buf_cap * 2;
								u8  *new_buf = push_array(arena, u8, new_cap);
								MemoryCopy(new_buf, buf, buf_pos);
								buf = new_buf;
								buf_cap = new_cap;
							}
							MemoryCopy(buf + buf_pos, str, len);
							buf_pos += len;
						} else {
							MemoryCopy(buf + buf_pos, "(null)", 6);
							buf_pos += 6;
						}
						break;
					}
					case 'S': {
						String str = va_arg(args, String);
						if (str.str && str.size > 0) {
							while (buf_pos + str.size >= buf_cap) {
								u64  new_cap = buf_cap * 2;
								u8  *new_buf = push_array(arena, u8, new_cap);
								MemoryCopy(new_buf, buf, buf_pos);
								buf = new_buf;
								buf_cap = new_cap;
							}
							MemoryCopy(buf + buf_pos, str.str, str.size);
							buf_pos += str.size;
						}
						break;
					}
					case 'd': {
						s64 val = va_arg(args, int);
						u32 len = fmt_s64_to_str(val, num_buf);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'u': {
						u64 val = va_arg(args, unsigned int);
						u32 len = fmt_u64_to_str(val, num_buf, 10);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'x': {
						u64 val = va_arg(args, unsigned int);
						u32 len = fmt_u64_to_hex(val, num_buf, 0);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'X': {
						u64 val = va_arg(args, unsigned int);
						u32 len = fmt_u64_to_hex(val, num_buf, 1);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'b': {
						u64 val = va_arg(args, unsigned int);
						u32 len = fmt_u64_to_bin(val, num_buf);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'f': {
						f64 val = va_arg(args, double);
						u32 len = fmt_f64_to_str(val, num_buf, 6);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'p': {
						void *ptr = va_arg(args, void *);
						buf[buf_pos++] = '0';
						buf[buf_pos++] = 'x';
						u32 len = fmt_u64_to_hex((u64)(uintptr_t)ptr, num_buf, 0);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'o': {
						va_arg(args, void *);
						MemoryCopy(buf + buf_pos, "{ object }", 10);
						buf_pos += 10;
						break;
					}
					default: {
						buf[buf_pos++] = '{';
						buf[buf_pos++] = spec;
						buf[buf_pos++] = '}';
						break;
					}
                }
				
                p += 3;
                continue;
            }
        }
		
        if (*p == '}' && *(p + 1) == '}') {
            buf[buf_pos++] = '}';
            p += 2;
            continue;
        }
		
        buf[buf_pos++] = *p++;
    }
	
    buf[buf_pos] = '\0';
    return (String){buf, buf_pos};
}

String
str_fmt(Arena *arena, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String result = str_fmtv(arena, fmt, args);
    va_end(args);
    return result;
}

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
static void print_raw(const char *data, u64 len) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(h, data, (DWORD)len, &written, NULL);
    }
}
#else
#include <unistd.h>
static void print_raw(const char *data, u64 len) {
    write(STDOUT_FILENO, data, len);
}
#endif

void
printv(const char *fmt, va_list args) {
    char stack_buf[4096];
    char *buf = stack_buf;
    u64   buf_cap = sizeof(stack_buf);
    u64   buf_pos = 0;
    char  num_buf[128];
    const char *p = fmt;
	
    while (*p) {
        if (buf_pos + 128 > buf_cap) {
            print_raw(buf, buf_pos);
            buf_pos = 0;
        }
		
        if (*p == '{') {
            if (*(p + 1) == '{') {
                buf[buf_pos++] = '{';
                p += 2;
                continue;
            }
			
            const char *end = p + 1;
            while (*end && *end != '}') end++;
			
            if (*end == '}') {
                char spec = *(p + 1);
				
                switch (spec) {
					case 's': {
						const char *str = va_arg(args, const char *);
						if (str) {
							u64 len = 0;
							while (str[len]) len++;
							if (buf_pos > 0) { print_raw(buf, buf_pos); buf_pos = 0; }
							print_raw(str, len);
						} else {
							MemoryCopy(buf + buf_pos, "(null)", 6);
							buf_pos += 6;
						}
						break;
					}
					case 'S': {
						String str = va_arg(args, String);
						if (str.str && str.size > 0) {
							if (buf_pos > 0) { print_raw(buf, buf_pos); buf_pos = 0; }
							print_raw((const char *)str.str, str.size);
						}
						break;
					}
					case 'd': {
						s64 val = va_arg(args, int);
						u32 len = fmt_s64_to_str(val, num_buf);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'u': {
						u64 val = va_arg(args, unsigned int);
						u32 len = fmt_u64_to_str(val, num_buf, 10);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'x': {
						u64 val = va_arg(args, unsigned int);
						u32 len = fmt_u64_to_hex(val, num_buf, 0);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'X': {
						u64 val = va_arg(args, unsigned int);
						u32 len = fmt_u64_to_hex(val, num_buf, 1);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'b': {
						u64 val = va_arg(args, unsigned int);
						u32 len = fmt_u64_to_bin(val, num_buf);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'f': {
						f64 val = va_arg(args, double);
						u32 len = fmt_f64_to_str(val, num_buf, 6);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'p': {
						void *ptr = va_arg(args, void *);
						buf[buf_pos++] = '0';
						buf[buf_pos++] = 'x';
						u32 len = fmt_u64_to_hex((u64)(uintptr_t)ptr, num_buf, 0);
						MemoryCopy(buf + buf_pos, num_buf, len);
						buf_pos += len;
						break;
					}
					case 'o': {
						va_arg(args, void *);
						MemoryCopy(buf + buf_pos, "{ object }", 10);
						buf_pos += 10;
						break;
					}
					default: {
						buf[buf_pos++] = '{';
						buf[buf_pos++] = spec;
						buf[buf_pos++] = '}';
						break;
					}
                }
				
                p += 3;
                continue;
            }
        }
		
        if (*p == '}' && *(p + 1) == '}') {
            buf[buf_pos++] = '}';
            p += 2;
            continue;
        }
		
        buf[buf_pos++] = *p++;
    }
	
    if (buf_pos > 0) print_raw(buf, buf_pos);
}

void
print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printv(fmt, args);
    va_end(args);
}
