#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

u64
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
            simd_v4s32 multiplier = simd_set1_s32(mult);
            u64 p = lo_prefix;
            u64 i = 0;

            for (; i + 4 <= count; i += 4) {
                simd_v4s32 prefixes = simd_set_s32((s32)p, (s32)(p+1), (s32)(p+2), (s32)(p+3));
                simd_v4s32 products = simd_mul_s32(prefixes, multiplier);
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

    char buf[32];
    u32 len = fmt_u64_to_str(total_sum, buf, 10);
    buf[len] = 0;
    print("Part 1 ({s}): {s} (time: {u} us)\n",
          use_simd ? "SIMD" : "scalar", buf, (u32)elapsed_us);
}

void
solve_part2(Arena *arena, String input) {
    print("Part 2: {s}\n", "not implemented");
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
    solve_part2(arena, input);

    arena_release(arena);
}
