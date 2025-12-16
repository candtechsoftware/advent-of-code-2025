#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

static inline u32
digit_count_u64(u64 n) {
    if (n == 0) return 1;
    u32 count = 0;
    while (n > 0) {
        n /= 10;
        count++;
    }
    return count;
}

static inline u64
pow_u64(u64 base, u32 exp) {
    u64 result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

static inline u64
parse_number(u8 **ptr, u8 *end) {
    u64 n = 0;
    while (*ptr < end && char_is_digit(**ptr)) {
        n = n * 10 + (**ptr - '0');
        (*ptr)++;
    }
    return n;
}

u64
solve_range_scalar(u64 start, u64 end) {
    u64 sum = 0;
    u32 end_digits = digit_count_u64(end);

    for (u32 digits = 2; digits <= end_digits; digits += 2) {
        u32 half = digits / 2;
        u64 divisor = pow_u64(10, half);

        u64 min_prefix = pow_u64(10, half - 1);
        if (half == 1) min_prefix = 1;
        u64 max_prefix = divisor - 1;

        u64 first_doubled = min_prefix * divisor + min_prefix;
        u64 last_doubled = max_prefix * divisor + max_prefix;

        if (last_doubled < start || first_doubled > end) continue;

        u64 lo_prefix = (start <= first_doubled) ? min_prefix : (start / divisor);
        if (lo_prefix * divisor + lo_prefix < start) lo_prefix++;

        u64 hi_prefix = (end >= last_doubled) ? max_prefix : (end / divisor);
        if (hi_prefix * divisor + hi_prefix > end) hi_prefix--;

        if (lo_prefix <= hi_prefix) {
            for (u64 p = lo_prefix; p <= hi_prefix; p++) {
                sum += p * divisor + p;
            }
        }
    }
    return sum;
}

u64
solve_range_simd(u64 start, u64 end) {
    u64 sum = 0;
    u32 end_digits = digit_count_u64(end);

    for (u32 digits = 2; digits <= end_digits; digits += 2) {
        u32 half = digits / 2;
        u64 divisor = pow_u64(10, half);

        u64 min_prefix = pow_u64(10, half - 1);
        if (half == 1) min_prefix = 1;
        u64 max_prefix = divisor - 1;

        u64 first_doubled = min_prefix * divisor + min_prefix;
        u64 last_doubled = max_prefix * divisor + max_prefix;

        if (last_doubled < start || first_doubled > end) continue;

        u64 lo_prefix = (start <= first_doubled) ? min_prefix : (start / divisor);
        if (lo_prefix * divisor + lo_prefix < start) lo_prefix++;

        u64 hi_prefix = (end >= last_doubled) ? max_prefix : (end / divisor);
        if (hi_prefix * divisor + hi_prefix > end) hi_prefix--;

        if (lo_prefix <= hi_prefix) {
            u64 count = hi_prefix - lo_prefix + 1;
            s32 mult = (s32)(divisor + 1);

#if USE_NEON || USE_SSE4 || USE_AVX2
            Simd_V4s32 multiplier = simd_set1_s32(mult);
            u64 p = lo_prefix;
            u64 i = 0;

            for (; i + 4 <= count; i += 4) {
                Simd_V4s32 prefixes = simd_set_s32((s32)p, (s32)(p+1), (s32)(p+2), (s32)(p+3));
                Simd_V4s32 products = simd_mul_s32(prefixes, multiplier);
                sum += (u64)simd_hsum_s32(products);
                p += 4;
            }

            for (; i < count; i++) {
                sum += p * mult;
                p++;
            }
#else
            for (u64 p = lo_prefix; p <= hi_prefix; p++) {
                sum += p * mult;
            }
#endif
        }
    }

    return sum;
}

void
solve_part1(Arena *arena, String input, b32 use_simd) {
    u64 total_sum = 0;

    u8 *ptr = input.str;
    u8 *end = input.str + input.size;

    u64 start_time = os_now_microseconds();

    while (ptr < end) {
        while (ptr < end && (char_is_whitespace(*ptr) || *ptr == ',')) ptr++;
        if (ptr >= end) break;

        u64 range_start = parse_number(&ptr, end);
        if (ptr < end && *ptr == '-') ptr++;
        u64 range_end = parse_number(&ptr, end);

        if (range_end >= range_start) {
            if (use_simd) {
                total_sum += solve_range_simd(range_start, range_end);
            } else {
                total_sum += solve_range_scalar(range_start, range_end);
            }
        }
    }

    u64 end_time = os_now_microseconds();
    u64 elapsed_us = end_time - start_time;

    // @TOOD(Alex): Work on your fmt library this is trash
    char buf[32];
    u32 len = fmt_u64_to_str(total_sum, buf, 10);
    buf[len] = 0;
    print("Part 1 ({s}): {s} (time: {u} us)\n",
          use_simd ? "SIMD" : "scalar", buf, (u32)elapsed_us);
}

static b32
is_repeated_pattern(u64 n) {
    if (n < 10) return 0;

    u32 num_digits = digit_count_u64(n);

    for (u32 pattern_len = 1; pattern_len <= num_digits / 2; pattern_len++) {
        if (num_digits % pattern_len != 0) continue;

        u64 divisor = pow_u64(10, pattern_len);
        u64 pattern = n % divisor;
        u64 temp = n;
        b32 valid = 1;

        while (temp > 0) {
            if ((temp % divisor) != pattern) {
                valid = 0;
                break;
            }
            temp /= divisor;
        }

        if (valid) return 1;
    }

    return 0;
}

u64
solve_range_part2_scalar_slow(u64 start, u64 end) {
    u64 sum = 0;
    for (u64 n = start; n <= end; n++) {
        if (is_repeated_pattern(n)) {
            sum += n;
        }
    }
    return sum;
}

static b32
is_itself_repeated(u64 pattern, u32 pattern_len) {
    if (pattern_len < 2) return 0;
    for (u32 sub_len = 1; sub_len < pattern_len; sub_len++) {
        if (pattern_len % sub_len != 0) continue;
        u64 divisor = pow_u64(10, sub_len);
        u64 sub_pattern = pattern % divisor;
        u64 temp = pattern;
        b32 valid = 1;
        while (temp > 0) {
            if ((temp % divisor) != sub_pattern) { valid = 0; break; }
            temp /= divisor;
        }
        if (valid) return 1;
    }
    return 0;
}

u64
solve_range_part2_scalar_fast(u64 start, u64 end) {
    u64 sum = 0;
    u32 max_digits = digit_count_u64(end);

    for (u32 total_digits = 2; total_digits <= max_digits; total_digits++) {
        for (u32 pattern_len = 1; pattern_len <= total_digits / 2; pattern_len++) {
            if (total_digits % pattern_len != 0) continue;
            u32 repeats = total_digits / pattern_len;
            if (repeats < 2) continue;

            u64 base = pow_u64(10, pattern_len);
            u64 multiplier = 0;
            for (u32 r = 0; r < repeats; r++) {
                multiplier += pow_u64(base, r);
            }

            u64 min_pattern = (pattern_len == 1) ? 1 : pow_u64(10, pattern_len - 1);
            u64 max_pattern = base - 1;

            for (u64 p = min_pattern; p <= max_pattern; p++) {
                if (is_itself_repeated(p, pattern_len)) continue;
                u64 num = p * multiplier;
                if (num < start) continue;
                if (num > end) break;
                sum += num;
            }
        }
    }
    return sum;
}

u64
solve_range_part2_simd(u64 start, u64 end) {
    u64 sum = 0;
    u32 max_digits = digit_count_u64(end);

    for (u32 total_digits = 2;
            total_digits <= max_digits;
            total_digits++) {
        for (u32 pattern_len = 1;
                pattern_len <= total_digits / 2;
                pattern_len++) {

            if (total_digits % pattern_len != 0) continue;
            u32 repeats = total_digits / pattern_len;
            if (repeats < 2) continue;

            u64 base = pow_u64(10, pattern_len);
            u64 multiplier = 0;
            for (u32 r = 0; r < repeats; r++) {
                multiplier += pow_u64(base, r);
            }

            u64 min_pattern = (pattern_len == 1) ? 1 : pow_u64(10, pattern_len - 1);
            u64 max_pattern = base - 1;

            u64 lo = min_pattern;
            u64 first_valid = lo * multiplier;
            if (first_valid < start) {
                lo = (start + multiplier - 1) / multiplier;
                if (lo < min_pattern) lo = min_pattern;
            }

            u64 hi = max_pattern;
            u64 last_valid = hi * multiplier;
            if (last_valid > end) {
                hi = end / multiplier;
                if (hi > max_pattern) hi = max_pattern;
            }
            if (lo > hi) continue;

            u64 p = lo;

#if USE_NEON || USE_SSE4 || USE_AVX2
            for (; p + 4 <= hi + 1; p += 4) {
                s32 skip[4];
                skip[0] = is_itself_repeated(p, pattern_len) ? 0 : 1;
                skip[1] = is_itself_repeated(p+1, pattern_len) ? 0 : 1;
                skip[2] = is_itself_repeated(p+2, pattern_len) ? 0 : 1;
                skip[3] = is_itself_repeated(p+3, pattern_len) ? 0 : 1;

                Simd_V4s32 mask = simd_loadu_s32(skip);
                Simd_V4s32 patterns = simd_set_s32((s32)p, (s32)(p+1), (s32)(p+2), (s32)(p+3));
                Simd_V4s32 mult_vec = simd_set1_s32((s32)multiplier);
                Simd_V4s32 nums = simd_mul_s32(patterns, mult_vec);
                Simd_V4s32 masked = simd_mul_s32(nums, mask);
                sum += (u64)simd_hsum_s32(masked);
            }
#endif
            for (; p <= hi; p++) {
                if (is_itself_repeated(p, pattern_len)) continue;
                sum += p * multiplier;
            }
        }
    }
    return sum;
}

// @Note(Alex): Playing around with funciton pointers 
// spend some times looking at compiler out put and 
// also see if there is a way to profile this as well 
// it is one way to do dynamic dispatch but it is akin to OOP 
// note sure I like it but it could be usefule and also could see how it compares
// normal switch/case or even if/else. but the syntax is quite nice?
typedef u64 (*Part2RangeFn)(u64 start, u64 end);

void
solve_part2(Arena *arena, String input, Part2RangeFn range_fn, char *label) {
    u64 total_sum = 0;

    u8 *ptr = input.str;
    u8 *end = input.str + input.size;

    u64 start_time = os_now_microseconds();

    while (ptr < end) {
        while (ptr < end && (char_is_whitespace(*ptr) || *ptr == ',')) ptr++;
        if (ptr >= end) break;

        u64 range_start = parse_number(&ptr, end);
        if (ptr < end && *ptr == '-') ptr++;
        u64 range_end = parse_number(&ptr, end);

        if (range_end >= range_start) {
            total_sum += range_fn(range_start, range_end);
        }
    }

    u64 end_time = os_now_microseconds();
    u64 elapsed_us = end_time - start_time;

    char buf[32];
    u32 len = fmt_u64_to_str(total_sum, buf, 10);
    buf[len] = 0;
    print("Part 2 ({s}): {s} (time: {u} us)\n", label, buf, (u32)elapsed_us);
}

void
entry_point(Cmd_Line *cmd_line) {
    Arena *arena = arena_alloc();
    log_init(arena, str_lit(""));

    String input_path = str_lit("inputs/day_02.txt");
    String input = os_data_from_file_path(arena, input_path);
    if (input.size == 0) {
        print("Error: Could not read {S}\n", input_path);
        return;
    }

    print("=== Day 2 ===\n");
    solve_part1(arena, input, 0);
    solve_part1(arena, input, 1);
    print("\n");
    // @NOTE(Alex): read above note do I like this? 
    solve_part2(arena, input, solve_range_part2_scalar_slow, "scalar_slow");
    solve_part2(arena, input, solve_range_part2_scalar_fast, "scalar_fast");
    solve_part2(arena, input, solve_range_part2_simd, "simd");

    arena_release(arena);
}
