#pragma once

#include "base.h"
#include "string_core.h"
#include "arena.h"
#include <stdarg.h>

// Format specifiers: {s} C string, {S} String, {d} int, {u} uint, {x}/{X} hex,
//                    {b} binary, {f} float, {p} pointer, {{ }} escaped braces

static String str_fmt(Arena *arena, const char *fmt, ...);
static String str_fmtv(Arena *arena, const char *fmt, va_list args);

static int str_vsnprintf(char *buf, int count, const char *fmt, va_list args);
static int str_snprintf(char *buf, int count, const char *fmt, ...);

static void print(const char *fmt, ...);
static void printv(const char *fmt, va_list args);

#define fmt(fmt_str, ...) str_fmt(tctx_get_scratch(0, 0), fmt_str, ##__VA_ARGS__)
