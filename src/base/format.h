#pragma once

#include "base.h"
#include "string_core.h"
#include "arena.h"
#include <stdarg.h>

// Format specifiers: {s} C string, {S} String, {d} int, {u} uint, {x}/{X} hex,
//                    {b} binary, {f} float, {p} pointer, {{ }} escaped braces

String str_fmt(Arena *arena, const char *fmt, ...);
String str_fmtv(Arena *arena, const char *fmt, va_list args);

void print(const char *fmt, ...);
void printv(const char *fmt, va_list args);

#define fmt(fmt_str, ...) str_fmt(tctx_get_scratch(0, 0), fmt_str, ##__VA_ARGS__)

u32 fmt_u64_to_str(u64 val, char *buf, u32 base);
u32 fmt_s64_to_str(s64 val, char *buf);
u32 fmt_f64_to_str(f64 val, char *buf, u32 precision);
