#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static inline b32 char_is_whitespace(u8 c) {
    return (c == ' ' || c == '\n' || c == '\t' ||
            c == '\r' || c == '\f' || c == '\v');
}

static inline b32 char_is_slash(u8 c) {
    return (c == '/' || c == '\\');
}

static inline b32 char_is_digit(u8 c) {
    return ('0' <= c && c <= '9');
}

static inline u8 char_to_upper(u8 c) {
    if ('a' <= c && c <= 'z') {
        c = (u8)(c + ('A' - 'a'));
    }
    return c;
}

static inline u8 char_to_lower(u8 c) {
    if ('A' <= c && c <= 'Z') {
        c += 'a' - 'A';
    }
    return c;
}

u64 string_count_char(String str, u8 c) {
    u64 count = 0;
    for (u64 i = 0; i < str.size; i++) {
        if (str.str[i] == c) {
            count++;
        }
    }
    return count;
}

String str_range(u8 *first, u8 *opl) {
    String result = {first, (u64)(opl - first)};
    return result;
}

String str_cstring(u8 *cstr) {
    u8 *ptr = cstr;
    for (; *ptr != 0; ptr += 1)
        ;
    String result = str_range(cstr, ptr);
    return result;
}

String str_cstring_uncapped(u8 *cstr, u8 *opl) {
    u8 *ptr = cstr;
    for (; ptr < opl && *ptr != 0; ptr += 1)
        ;
    String result = str_range(cstr, ptr);
    return result;
}

String16 str16_cstring(u16 *cstr) {
    u16 *ptr = cstr;
    for (; *ptr != 0; ptr += 1)
        ;
    String16 result = {cstr, (u64)(ptr - cstr)};
    return result;
}

static void 
str_list_push(Arena *arena, String_List *list, String string) {
    String_Node *node = push_array(arena, String_Node, 1);
    node->string = string;
    SLLQueuePush(list->first, list->last, node);
    list->count += 1;
    list->total_size += string.size;
}

static void 
str_list_push_front(Arena *arena, String_List *list, String string) {
    String_Node *node = push_array(arena, String_Node, 1);
    node->string = string;
    SLLQueuePushFront(list->first, list->last, node);
    list->count += 1;
    list->total_size += string.size;
}

static String_List 
str_list_copy(Arena *arena, String_List *list) {
    String_List result = {0};
    for (String_Node *node = list->first; node != 0; node = node->next) {
        String string = str_push_copy(arena, node->string);
        str_list_push(arena, &result, string);
    }
    return result;
}

static String 
str_list_join(Arena *arena, String_List *list, String_Join *options) {
    String_Join join = {0};
    if (options != 0) {
        MemoryCopyStruct(&join, options);
    }
    u64 sep_count = 0;
    if (list->count > 0) {
        sep_count = list->count - 1;
    }
    String result;
    result.size = join.pre.size + join.post.size + sep_count * join.mid.size + list->total_size;
    u8 *ptr = result.str = push_array_no_zero(arena, u8, result.size + 1);
    MemoryCopy(ptr, join.pre.str, join.pre.size);
    ptr += join.pre.size;
    for (String_Node *node = list->first;
         node != 0;
         node = node->next) {
        MemoryCopy(ptr, node->string.str, node->string.size);
        ptr += node->string.size;
        if (node->next != 0) {
            MemoryCopy(ptr, join.mid.str, join.mid.size);
            ptr += join.mid.size;
        }
    }
    MemoryCopy(ptr, join.post.str, join.post.size);
    ptr += join.post.size;
    *ptr = 0;
    return result;
}

static String 
str_join(Arena *arena, String_List *list) {
    u64 size = list->total_size;
    u8 *str = push_array(arena, u8, size + 1);
    u8 *ptr = str;
	
    for (String_Node *node = list->first; node != 0; node = node->next) {
        MemoryCopy(ptr, node->string.str, node->string.size);
        ptr += node->string.size;
    }
	
    *ptr = 0;
    String result = {str, size};
    return result;
}

static String_List 
str_split(Arena *arena, String string, u8 *split_char, u32 count) {
    String_List result = {0};
    u8         *start = string.str;
    u8         *ptr = string.str;
    u8         *end = string.str + string.size;
	
    while (ptr < end) {
        b32 is_split = 0;
        for (u32 i = 0; i < count; i++) {
            if (*ptr == split_char[i]) {
                is_split = 1;
                break;
            }
        }
		
        if (is_split) {
            if (ptr > start) {
                String part = {start, (u64)(ptr - start)};
                str_list_push(arena, &result, part);
            }
            ptr++;
            start = ptr;
        } else {
            ptr++;
        }
    }
	
    if (ptr > start) {
        String part = {start, (u64)(ptr - start)};
        str_list_push(arena, &result, part);
    }
	
    return result;
}


static String 
str_pushfv(Arena *arena, char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
	
    int needed = vsnprintf(0, 0, fmt, args_copy);
    va_end(args_copy);
	
    if (needed < 0) {
        String result = {0};
        return result;
    }
	
    u8 *str = push_array(arena, u8, needed + 1);
    vsnprintf((char *)str, needed + 1, fmt, args);
	
    String result = {str, (u64)needed};
    return result;
}

String str_pushf(Arena *arena, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String result = str_pushfv(arena, fmt, args);
    va_end(args);
    return result;
}

void str_list_pushf(Arena *arena, String_List *list, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String string = str_pushfv(arena, fmt, args);
    va_end(args);
    str_list_push(arena, list, string);
}

String str_push_copy(Arena *arena, String string) {
    u8 *str = push_array(arena, u8, string.size + 1);
    MemoryCopy(str, string.str, string.size);
    str[string.size] = 0;
    String result = {str, string.size};
    return result;
}

String str_prefix(String str, u64 size) {
    if (size > str.size) {
        size = str.size;
    }
    String result = {str.str, size};
    return result;
}

String str_chop(String str, u64 amount) {
    if (amount > str.size) {
        amount = str.size;
    }
    String result = {str.str, str.size - amount};
    return result;
}

String str_postfix(String str, u64 size) {
    if (size > str.size) {
        size = str.size;
    }
    u64    skip = str.size - size;
    String result = {str.str + skip, size};
    return result;
}

String str_skip(String str, u64 amount) {
    if (amount > str.size) {
        amount = str.size;
    }
    String result = {str.str + amount, str.size - amount};
    return result;
}

String str_skip_chop_whitespace(String str) {
    String result = {0};
    if (str.size > 0) {
        u8 *sptr = str.str;
        u8 *eptr = str.str + str.size - 1;
        for (; sptr <= eptr && char_is_whitespace(*sptr); sptr += 1)
            ;
        for (; eptr >= sptr && char_is_whitespace(*eptr); eptr -= 1)
            ;
        if (sptr <= eptr) {
            result = str_range(sptr, eptr + 1);
        }
    }
    return result;
}

static u64 
str_hash(String str) {
    u64 hash = 5381;
    for (u8 *ptr = str.str, *opl = str.str + str.size; ptr < opl; ptr += 1) {
        u8 c = *ptr;
        hash = (hash * 33) ^ c;
    }
    return hash;
}

static b32 
str_match(String a, String b, String_Match_Flags flags) {
    b32 result = 0;
    if ((flags & String_Match_Flag_Prefix_Match) != 0 || a.size == b.size) {
        u64 size = a.size;
        if ((flags & String_Match_Flag_Prefix_Match) != 0) {
            size = Min(a.size, b.size);
        }
		
        result = 1;
        b32 no_case = ((flags & String_Match_Flag_No_Case) != 0);
        for (u64 i = 0; i < size; i += 1) {
            u8 ac = a.str[i];
            u8 bc = b.str[i];
            if (no_case) {
                ac = char_to_upper(ac);
                bc = char_to_upper(bc);
            }
            if (ac != bc) {
                result = 0;
                break;
            }
        }
    }
    return result;
}

static String 
str_chop_last_slash(String string) {
    String result = string;
    if (string.size > 0) {
        u64 pos = string.size;
        for (s64 i = (s64)string.size - 1; i >= 0; i -= 1) {
            if (char_is_slash(string.str[i])) {
                pos = i;
                break;
            }
        }
        result.size = pos;
    }
    return result;
}

static String
str_file_name_from_path(String full_file_name) {
    String result = {0};
    if (full_file_name.size > 0) {
        u8 *opl = full_file_name.str + full_file_name.size;
        u8 *ptr = opl;
        for (; ptr > full_file_name.str; ptr -= 1) {
            u8 c = ptr[-1];
            if (c == '/' || c == '\\') {
                break;
            }
        }
        result = str_range(ptr, opl);
    }
    return result;
}

static String 
str_base_name_from_file_name(String file_name) {
    String result = {0};
    if (file_name.size > 0) {
        u8 *opl = file_name.str + file_name.size;
        u8 *ptr = file_name.str;
        for (; ptr < opl; ptr += 1) {
            u8 c = ptr[0];
            if (c == '.') {
                break;
            }
        }
        result = str_range(file_name.str, ptr);
    }
    return result;
}

static String 
str_join_flags(Arena *arena, String_List *list) {
    String_Join join = {0};
    join.mid = str_lit(" | ");
	
    u64 size = ((list->count > 0) ? (join.mid.size * (list->count - 1)) : 0) + list->total_size;
    u8 *str = push_array(arena, u8, size + 1);
    u8 *ptr = str;
	
    b32 is_mid = 0;
    for (String_Node *node = list->first; node != 0; node = node->next) {
        if (is_mid) {
            MemoryCopy(ptr, join.mid.str, join.mid.size);
            ptr += join.mid.size;
        }
        MemoryCopy(ptr, node->string.str, node->string.size);
        ptr += node->string.size;
        is_mid = 1;
    }
	
    *ptr = 0;
    String result = {str, size};
    if (result.size == 0) {
        result = str_lit("0");
    }
    return result;
}

static inline String_Decode
str_decode_utf8(u8 *str, u32 cap) {
    String_Decode result = {.codepoint = '#', .size = 1};
    if (cap == 0) return result;
	
    u8 byte = str[0];
    if (byte < 0x80) {
        result.codepoint = byte;
        return result;
    }
	
    if ((byte & 0xE0) == 0xC0 && cap > 1) {
        result.codepoint = ((u32)(byte & 0x1F) << 6) | (u32)(str[1] & 0x3F);
        result.size = 2;
    } else if ((byte & 0xF0) == 0xE0 && cap > 2) {
        result.codepoint = ((u32)(byte & 0x0F) << 12) |
		((u32)(str[1] & 0x3F) << 6) |
		(u32)(str[2] & 0x3F);
        result.size = 3;
    } else if ((byte & 0xF8) == 0xF0 && cap > 3) {
        result.codepoint = ((u32)(byte & 0x07) << 18) |
		((u32)(str[1] & 0x3F) << 12) |
		((u32)(str[2] & 0x3F) << 6) |
		(u32)(str[3] & 0x3F);
        result.size = 4;
    }
    return result;
}

static u32 
str_encode_utf8(u8 *dst, u32 codepoint) {
    u32 size = 0;
    if (codepoint < (1 << 7)) {
        dst[0] = (u8)codepoint;
        size = 1;
    } else if (codepoint < (1 << 11)) {
        dst[0] = 0xC0 | (codepoint >> 6);
        dst[1] = 0x80 | (codepoint & 0x3F);
        size = 2;
    } else if (codepoint < (1 << 16)) {
        dst[0] = 0xE0 | (codepoint >> 12);
        dst[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[2] = 0x80 | (codepoint & 0x3F);
        size = 3;
    } else if (codepoint < (1 << 21)) {
        dst[0] = 0xF0 | (codepoint >> 18);
        dst[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        dst[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[3] = 0x80 | (codepoint & 0x3F);
        size = 4;
    }
    return size;
}

static String_Decode 
str_decode_utf16(u16 *str, u32 cap) {
    String_Decode result = {0xFFFF, 1};
    if (cap > 0) {
        u16 x = str[0];
        result.codepoint = x;
        if (0xD800 <= x && x < 0xDC00 && cap > 1) {
            u16 y = str[1];
            if (0xDC00 <= y && y < 0xE000) {
                result.codepoint = ((x - 0xD800) << 10) | (y - 0xDC00);
                result.size = 2;
            }
        }
    }
    return result;
}

static String_Decode 
str_encode_utf16(u16 *str, u32 cap) {
    String_Decode result = {0xFFFF, 0};
    if (cap < 0x10000) {
        str[0] = (u16)cap;
        result.size = 1;
    } else if (cap < 0x110000) {
        u32 v = cap - 0x10000;
        str[0] = 0xD800 + (v >> 10);
        str[1] = 0xDC00 + (v & 0x3FF);
        result.size = 2;
    }
    return result;
}

static String32 
str32_from_str(Arena *arena, String string) {
    u64  cap = string.size;
    u32 *str = push_array(arena, u32, cap + 1);
    u8  *ptr = string.str;
    u8  *opl = string.str + string.size;
    u64  size = 0;
	
    for (; ptr < opl;) {
        String_Decode decode = str_decode_utf8(ptr, (u64)(opl - ptr));
        str[size++] = decode.codepoint;
        ptr += decode.size;
    }
	
    String32 result = {str, size};
    return result;
}

static String 
str_from_str32(Arena *arena, String32 string) {
    u64  cap = string.size * 4;
    u8  *str = push_array(arena, u8, cap + 1);
    u32 *ptr = string.str;
    u32 *opl = string.str + string.size;
    u64  size = 0;
	
    for (; ptr < opl; ptr += 1) {
        u32 enc_size = str_encode_utf8(str + size, *ptr);
        size += enc_size;
    }
	
    String result = {str, size};
    return result;
}

static String16 
str16_from_str(Arena *arena, String string) {
    u64  cap = string.size;
    u16 *str = push_array(arena, u16, cap + 1);
    u8  *ptr = string.str;
    u8  *opl = string.str + string.size;
    u64  size = 0;
	
    for (; ptr < opl;) {
        String_Decode decode = str_decode_utf8(ptr, (u64)(opl - ptr));
        String_Decode enc = str_encode_utf16(str + size, decode.codepoint);
        size += enc.size;
        ptr += decode.size;
    }
	
    String16 result = {str, size};
    return result;
}

static String 
str_from_str16(Arena *arena, String16 string) {
    u64  cap = string.size * 3;
    u8  *str = push_array(arena, u8, cap + 1);
    u16 *ptr = string.str;
    u16 *opl = string.str + string.size;
    u64  size = 0;
	
    for (; ptr < opl;) {
        String_Decode decode = str_decode_utf16(ptr, (u64)(opl - ptr));
        u32           enc_size = str_encode_utf8(str + size, decode.codepoint);
        size += enc_size;
        ptr += decode.size;
    }
	
    String result = {str, size};
    return result;
}

static b32 
str_is_u64(String string, u32 radix) {
    if (string.size == 0 || radix < 2 || radix > 36) {
        return 0;
    }
	
    for (u64 i = 0; i < string.size; i++) {
        u8  c = string.str[i];
        u32 digit = 0;
        if ('0' <= c && c <= '9') {
            digit = c - '0';
        } else if ('a' <= c && c <= 'z') {
            digit = 10 + (c - 'a');
        } else if ('A' <= c && c <= 'Z') {
            digit = 10 + (c - 'A');
        } else {
            return 0;
        }
        if (digit >= radix) {
            return 0;
        }
    }
    return 1;
}

static u64
u64_from_str(String string, u32 radix) {
    u64 result = 0;
    for (u64 i = 0; i < string.size; i++) {
        u8  c = string.str[i];
        u32 digit = 0;
        if ('0' <= c && c <= '9') {
            digit = c - '0';
        } else if ('a' <= c && c <= 'z') {
            digit = 10 + (c - 'a');
        } else if ('A' <= c && c <= 'Z') {
            digit = 10 + (c - 'A');
        }
        result = result * radix + digit;
    }
    return result;
}

static u64
u64_from_str_c_syntax(String string) {
    u64 result = 0;
    u32 radix = 10;
    u64 start = 0;
	
    if (string.size > 1 && string.str[0] == '0') {
        if (string.str[1] == 'x' || string.str[1] == 'X') {
            radix = 16;
            start = 2;
        } else if (string.str[1] == 'b' || string.str[1] == 'B') {
            radix = 2;
            start = 2;
        } else {
            radix = 8;
            start = 1;
        }
    }
	
    String sub = {string.str + start, string.size - start};
    result = u64_from_str(sub, radix);
    return result;
}

static s64
s64_from_str_c_syntax(String string) {
    b32 negative = 0;
    u64 start = 0;
	
    if (string.size > 0 && string.str[0] == '-') {
        negative = 1;
        start = 1;
    } else if (string.size > 0 && string.str[0] == '+') {
        start = 1;
    }
	
    String sub = {string.str + start, string.size - start};
    u64    value = u64_from_str_c_syntax(sub);
    s64    result = (s64)value;
    if (negative) {
        result = -result;
    }
    return result;
}

static f64
f64_from_str(String string) {
    char buffer[256];
    u64  len = string.size < 255 ? string.size : 255;
    for (u64 i = 0; i < len; i++) {
        buffer[i] = (char)string.str[i];
    }
    buffer[len] = 0;
    return strtod(buffer, 0);
}

#if !defined(XXH_IMPLEMENTATION)
#    define XXH_INLINE_ALL
#    define XXH_IMPLEMENTATION
#    define XXH_STATIC_LINKING_ONLY
#    include "third_party/xxhash/xxhash.h"
#endif

static u64 
u64_hash_from_seed_str(u64 seed, String string) {
    u64 res = XXH3_64bits_withSeed(string.str, string.size, seed);
    return res;
}

static u64 
u64_hash_from_str(String string) {
    u64 res = u64_hash_from_seed_str(5381, string);
    return res;
}

u64 str_find_needle(String string, u64 start_pos, String needle, String_Match_Flags flags) {
    u8 *p = string.str + start_pos;
    u64 stop_offset = Max(string.size + 1, needle.size) - needle.size;
    u8 *stop_p = string.str + stop_offset;
    if (needle.size > 0) {
        u8                *string_opl = string.str + string.size;
        String             needle_tail = str_skip(needle, 1);
        String_Match_Flags adjusted_flags = flags | String_Match_Flag_Prefix_Match;
        u8                 needle_first_char_adjusted = needle.str[0];
        if (adjusted_flags & String_Match_Flag_No_Case) {
            needle_first_char_adjusted = char_to_upper(needle_first_char_adjusted);
        }
        for (; p < stop_p; p += 1) {
            u8 haystack_char_adjusted = *p;
            if (adjusted_flags & String_Match_Flag_No_Case) {
                haystack_char_adjusted = char_to_upper(haystack_char_adjusted);
            }
            if (haystack_char_adjusted == needle_first_char_adjusted) {
                if (str_match(str_range(p + 1, string_opl), needle_tail, adjusted_flags)) {
                    break;
                }
            }
        }
    }
    u64 result = string.size;
    if (p < stop_p) {
        result = (u64)(p - string.str);
    }
    return result;
}
