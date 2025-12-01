#pragma once
#include "base.h"
#include "arena.h"

#if LANG_CXX
#    define CLiteral(T) T
#else
#    define CLiteral(T) (T)
#endif

typedef struct String String;
struct String {
    u8 *str;
    u64 size;
};

typedef struct String_Node String_Node;
struct String_Node {
    String_Node *next;
    String       string;
};

typedef struct String_List String_List;
struct String_List {
    String_Node *first;
    String_Node *last;
    u64          count;
    u64          total_size;
};

typedef struct String_Array String_Array;
struct String_Array {
    String *v;
    u64     count;
    u64     total_size;
};

typedef struct String_Join String_Join;
struct String_Join {
    String pre;
    String mid;
    String post;
};

typedef u32 String_Match_Flags;
enum {
    String_Match_Flag_No_Case = (1 << 0),
    String_Match_Flag_Prefix_Match = (1 << 1),
};

typedef struct String16 String16;
struct String16 {
    u16 *str;
    u64  size;
};

typedef struct String32 String32;
struct String32 {
    u32 *str;
    u64  size;
};

#define str_expand(s) (int)((s).size), ((s).str)

typedef struct String_Decode String_Decode;
struct String_Decode {
    u32 codepoint;
    u32 size;
};

static inline b32 char_is_whitespace(u8 c);
static inline b32 char_is_slash(u8 c);
static inline b32 char_is_digit(u8 c);
static inline b32 char_is_hex_digit(u8 c);
static inline b32 char_is_alpha(u8 c);

static inline u8 char_to_upper(u8 c);
static inline u8 char_to_lower(u8 c);

#define str(p, z) CLiteral(String){(u8 *)(p), (z)};

String str_range(u8 *first, u8 *opl);
String str_cstring(u8 *cstr);
String str_cstring_uncapped(u8 *cstr, u8 *opl);

u64 string_count_char(String str, u8 c);

String16 str16_cstring(u16 *cstr);

#define str_lit(s)      CLiteral(String){(u8 *)(s), sizeof(s) - 1}
#define str_lit_comp(s) {(u8 *)(s), sizeof(s) - 1}

static void str_list_push(Arena *arena, String_List *list, String string);
static void str_list_push_front(Arena *arena, String_List *list, String string);
static u64  u64_hash_from_str(String string);

static String_List str_list_copy(Arena *arena, String_List *list);
static String      str_join(Arena *arena, String_List *list);
static String      str_list_join(Arena *arena, String_List *list, String_Join *options);
static String_List str_split(Arena *arena, String string, u8 *split_char, u32 count);

static String str_pushfv(Arena *arena, char *fmt, va_list args);
String        str_pushf(Arena *arena, char *fmt, ...);
void          str_list_pushf(Arena *arena, String_List *list, char *fmt, ...);
String        str_push_copy(Arena *arena, String string);

u64 str_find_needle(String string, u64 start_pos, String needle, String_Match_Flags flags);

String str_prefix(String str, u64 size);
String str_chop(String str, u64 amount);
String str_postfix(String str, u64 size);
String str_skip(String str, u64 amount);
String str_skip_chop_whitespace(String str);

static u64 str_hash(String str);
static b32 str_match(String a, String b, String_Match_Flags flags);

static String str_chop_last_slash(String string);
static String str_file_name_from_path(String full_file_name);
static String str_base_name_from_file_name(String file_name);

static String str_join_flags(Arena *arena, String_List *list);

static String_Decode str_decode_utf8(u8 *str, u32 cap);
static u32           str_encode_utf8(u8 *dst, u32 codepoint);
static String_Decode str_decode_utf16(u16 *str, u32 cap);
static String_Decode str_encode_utf16(u16 *str, u32 cap);

static String32 str32_from_str(Arena *arena, String string);
static String   str_from_str32(Arena *arena, String32 string);
static String16 str16_from_str(Arena *arena, String string);
static String   str_from_str16(Arena *arena, String16 string);

static b32 str_is_u64(String string, u32 radix);
static u64 u64_from_str(String string, u32 radix);
static u64 u64_from_str_c_syntax(String string);
static s64 s64_from_str_c_syntax(String string);
static f64 f64_from_str(String string);
