#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

static u64
solve_scalar_slow(String input) {
    u64 total = 0;
    u8 *ptr = input.str;
    u8 *end = input.str + input.size;

    while (ptr < end) {
        u8 *line_start = ptr;
        while (ptr < end && *ptr != '\n') ptr++;
        u64 line_len = ptr - line_start;
        if (ptr < end) ptr++;

        if (line_len < 2) continue;

        s32 max_joltage = 0;
        for (u64 i = 0; i < line_len - 1; i++) {
            s32 first = line_start[i] - '0';
            for (u64 j = i + 1; j < line_len; j++) {
                s32 second = line_start[j] - '0';
                s32 joltage = first * 10 + second;
                if (joltage > max_joltage) max_joltage = joltage;
            }
        }
        total += max_joltage;
    }

    return total;
}

static u64
solve_scalar_fast(String input) {
    u64 total = 0;
    u8 *ptr = input.str;
    u8 *end = input.str + input.size;

    u8 suffix_max[4096];

    while (ptr < end) {
        u8 *line_start = ptr;
        while (ptr < end && *ptr != '\n') ptr++;
        u64 line_len = ptr - line_start;
        if (ptr < end) ptr++;

        if (line_len < 2) continue;

        u8 running_max = line_start[line_len - 1] - '0';
        suffix_max[line_len - 1] = running_max;
        for (s64 i = (s64)line_len - 2; i >= 0; i--) {
            u8 digit = line_start[i] - '0';
            if (digit > running_max) running_max = digit;
            suffix_max[i] = running_max;
        }

        s32 max_joltage = 0;
        for (u64 i = 0; i < line_len - 1; i++) {
            s32 first = line_start[i] - '0';
            s32 best_second = suffix_max[i + 1];
            s32 joltage = first * 10 + best_second;
            if (joltage > max_joltage) max_joltage = joltage;
        }
        total += max_joltage;
    }

    return total;
}

static u64
solve_simd(String input) {
    u64 total = 0;
    u8 *ptr = input.str;
    u8 *end = input.str + input.size;

    u8 suffix_max[4096];

    while (ptr < end) {
        u8 *line_start = ptr;
        while (ptr < end && *ptr != '\n') ptr++;
        u64 line_len = ptr - line_start;
        if (ptr < end) ptr++;

        if (line_len < 2) continue;

        u8 running_max = line_start[line_len - 1] - '0';
        suffix_max[line_len - 1] = running_max;
        for (s64 j = (s64)line_len - 2; j >= 0; j--) {
            u8 digit = line_start[j] - '0';
            if (digit > running_max) running_max = digit;
            suffix_max[j] = running_max;
        }

        s32 max_joltage = 0;
        for (u64 i = 0; i < line_len - 1; i++) {
            s32 first = line_start[i] - '0';
            s32 best_second = suffix_max[i + 1];
            s32 joltage = first * 10 + best_second;
            if (joltage > max_joltage) max_joltage = joltage;
        }
        total += max_joltage;
    }

    return total;
}

static u64
solve_part2_scalar(String input) {
    u64 total = 0;
    u8 *ptr = input.str;
    u8 *end = input.str + input.size;

    u8 result[4096];

    while (ptr < end) {
        u8 *line_start = ptr;
        while (ptr < end && *ptr != '\n') ptr++;
        u64 line_len = ptr - line_start;
        if (ptr < end) ptr++;

        if (line_len <= 2) continue;

        for (u64 i = 0; i < line_len; i++) {
            result[i] = line_start[i];
        }
        u64 result_len = line_len;
        u64 to_remove = (line_len > 12) ? (line_len - 12) : 0;

        for (u64 remove = 0; remove < to_remove && result_len > 0; remove++) {
            b32 removed = 0;
            for (u64 i = 0; i < result_len - 1; i++) {
                if (result[i] < result[i + 1]) {
                    for (u64 j = i; j < result_len - 1; j++) {
                        result[j] = result[j + 1];
                    }
                    result_len--;
                    removed = 1;
                    break;
                }
            }
            if (!removed) {
                result_len--;
            }
        }

        u64 num = 0;
        for (u64 i = 0; i < result_len; i++) {
            num = num * 10 + (result[i] - '0');
        }
        total += num;
    }

    return total;
}

void
entry_point(Cmd_Line *cmd_line) {
    Arena *arena = arena_alloc();
    log_init(arena, str_lit(""));

    String input_path = str_lit("inputs/day_03.txt");
    String input = os_data_from_file_path(arena, input_path);
    if (input.size == 0) {
        print("Error: Could not read {S}\n", input_path);
        return;
    }

    print("=== Day 3 ===\n");

    u64 start, elapsed;
    u64 result;

    start = os_now_microseconds();
    result = solve_scalar_slow(input);
    elapsed = os_now_microseconds() - start;
    print("Part 1 (scalar_slow): {u} (time: {u} us)\n", (u32)result, (u32)elapsed);

    start = os_now_microseconds();
    result = solve_scalar_fast(input);
    elapsed = os_now_microseconds() - start;
    print("Part 1 (scalar_fast): {u} (time: {u} us)\n", (u32)result, (u32)elapsed);

    start = os_now_microseconds();
    result = solve_simd(input);
    elapsed = os_now_microseconds() - start;
    print("Part 1 (simd):        {u} (time: {u} us)\n", (u32)result, (u32)elapsed);

    print("\n");

    char buf[32];
    u32 len;

    start = os_now_microseconds();
    result = solve_part2_scalar(input);
    elapsed = os_now_microseconds() - start;
    len = fmt_u64_to_str(result, buf, 10);
    buf[len] = 0;
    print("Part 2 (scalar):      {s} (time: {u} us)\n", buf, (u32)elapsed);

    arena_release(arena);
}
