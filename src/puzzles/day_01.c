#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

typedef u32 Run_Flag;
enum {
    Run_Scalar_Slow = (1<<0),
    Run_Scalar_Fast = (1<<1),
    Run_Simd        = (1<<2),
    Run_Scalar      = Run_Scalar_Slow | Run_Scalar_Fast,
}; 

static u64
solve_scalar(s32 *directions, s32 *distances, u64 count) {
    s32 pos = 50;
    u64 zero_count = 0;

    for (u64 i = 0; i < count; i++) {
        s32 delta = directions[i] ? -distances[i] : distances[i];
        pos = (pos + delta) % 100;
        if (pos < 0) pos += 100;
        if (pos == 0) zero_count++;
    }

    return zero_count;
}

static u64
solve_simd(s32 *directions, s32 *distances, u64 count) {
    s32 pos = 50;
    u64 zero_count = 0;

    u64 i = 0;
#if USE_NEON || USE_SSE4 || USE_AVX2
    for (; i + 4 <= count; i += 4) {
        Simd_V4s32 dir_vec = simd_loadu_s32(&directions[i]);
        Simd_V4s32 dist_vec = simd_loadu_s32(&distances[i]);

        Simd_V4s32 neg_dist = simd_neg_s32(dist_vec);
        Simd_V4s32 delta = simd_blend_s32(dist_vec, neg_dist, dir_vec);

        s32 deltas[4];
        simd_storeu_s32(deltas, delta);

        for (int j = 0; j < 4; j++) {
            pos = (pos + deltas[j]) % 100;
            if (pos < 0) pos += 100;
            if (pos == 0) zero_count++;
        }
    }
#endif

    for (; i < count; i++) {
        s32 delta = directions[i] ? -distances[i] : distances[i];
        pos = (pos + delta) % 100;
        if (pos < 0) pos += 100;
        if (pos == 0) zero_count++;
    }

    return zero_count;
}

static void
solve_part1(Arena *arena, String input, b32 use_simd) {
    u64 line_count = 0;
    for (u64 i = 0; i < input.size; i++) {
        if (input.str[i] == '\n') line_count++;
    }

    s32 *directions = push_array(arena, s32, line_count);
    s32 *distances = push_array(arena, s32, line_count);

    u64 idx = 0;
    u8 *ptr = input.str;
    u8 *end = input.str + input.size;

    while (ptr < end && idx < line_count) {
        u8 dir = *ptr++;
        directions[idx] = (dir == 'L') ? -1 : 0;

        s32 num = 0;
        while (ptr < end && char_is_digit(*ptr)) {
            num = num * 10 + (*ptr++ - '0');
        }
        distances[idx] = num;

        while (ptr < end && char_is_whitespace(*ptr)) ptr++;
        idx++;
    }

    u64 zero_count;
    u64 start_time = os_now_microseconds();

    if (use_simd) {
        zero_count = solve_simd(directions, distances, idx);
    } else {
        zero_count = solve_scalar(directions, distances, idx);
    }

    u64 end_time = os_now_microseconds();
    u64 elapsed_us = end_time - start_time;

    print("Part 1 ({s}): {u} (time: {u} us)\n",
          use_simd ? "SIMD" : "scalar",
          (u32)zero_count,
          (u32)elapsed_us);
}

static u64
solve_scalar_part2_slow(s32 *directions, s32 *distances, u64 count) {
    s32 pos = 50;
    u64 zero_count = 0;

    for (u64 i = 0; i < count; i++) {
        s32 delta = directions[i] ? -distances[i] : distances[i];
        s32 dist = distances[i];

        if (delta > 0) {
            for (s32 step = 1; step <= dist; step++) {
                pos = (pos + 1) % 100;
                if (pos == 0) zero_count++;
            }
        } else {
            for (s32 step = 1; step <= dist; step++) {
                pos = (pos - 1 + 100) % 100;
                if (pos == 0) zero_count++;
            }
        }
    }

    return zero_count;
}

static u64
solve_scalar_part2(s32 *directions, s32 *distances, u64 count) {
    s32 pos = 50;
    u64 zero_count = 0;

    for (u64 i = 0; i < count; i++) {
        s32 dist = distances[i];
        b32 is_left = directions[i];

        if (!is_left) {
            zero_count += (pos + dist) / 100;
            pos = (pos + dist) % 100;
        } else {
            if (dist < pos) {
                pos = pos - dist;
            } else {
                s32 remaining = dist - pos;
                if (pos > 0) {
                    zero_count += 1;
                    zero_count += remaining / 100;
                    pos = (100 - (remaining % 100)) % 100;
                } else {
                    zero_count += remaining / 100;
                    pos = (100 - (remaining % 100)) % 100;
                }
            }
        }
    }

    return zero_count;
}

static u64
solve_simd_part2(s32 *directions, s32 *distances, u64 count) {
    s32 pos = 50;
    u64 zero_count = 0;

    for (u64 i = 0; i < count; i++) {
        s32 dist = distances[i];
        b32 is_left = directions[i];

        if (!is_left) {
            zero_count += (pos + dist) / 100;
            pos = (pos + dist) % 100;
        } else {
            if (dist < pos) {
                pos = pos - dist;
            } else {
                s32 remaining = dist - pos;
                if (pos > 0) {
                    zero_count += 1;
                    zero_count += remaining / 100;
                    pos = (100 - (remaining % 100)) % 100;
                } else {
                    zero_count += remaining / 100;
                    pos = (100 - (remaining % 100)) % 100;
                }
            }
        }
    }

    return zero_count;
}

static void
solve_part2(Arena *arena, String input, Run_Flag flag) {
    u64 line_count = 0;
    for (u64 i = 0; i < input.size; i++) {
        if (input.str[i] == '\n') line_count++;
    }

    s32 *directions = push_array(arena, s32, line_count);
    s32 *distances = push_array(arena, s32, line_count);

    u64 idx = 0;
    u8 *ptr = input.str;
    u8 *end = input.str + input.size;

    while (ptr < end && idx < line_count) {
        u8 dir = *ptr++;
        directions[idx] = (dir == 'L') ? -1 : 0;

        s32 num = 0;
        while (ptr < end && char_is_digit(*ptr)) {
            num = num * 10 + (*ptr++ - '0');
        }
        distances[idx] = num;

        while (ptr < end && char_is_whitespace(*ptr)) ptr++;
        idx++;
    }

    u64 zero_count = 0;
    u64 start_time = os_now_microseconds();

    if (flag & Run_Scalar_Slow) zero_count = solve_scalar_part2_slow(directions, distances, idx);
    if (flag & Run_Scalar_Fast) zero_count = solve_scalar_part2(directions, distances, idx);
    if (flag & Run_Simd) zero_count = solve_simd_part2(directions, distances, idx);

    u64 end_time = os_now_microseconds();
    u64 elapsed_us = end_time - start_time;

    String name = str_lit("unknown");
    if (flag & Run_Scalar_Slow) name = str_lit("scalar slow");
    else if (flag & Run_Scalar_Fast) name = str_lit("scalar fast");
    else if (flag & Run_Simd) name = str_lit("SIMD");

    print("Part 2 ({S}): {u} (time: {u} us)\n",
          name,
          (u32)zero_count,
          (u32)elapsed_us);
}

void
entry_point(Cmd_Line *cmd_line) {
    Arena *arena = arena_alloc();
    log_init(arena, str_lit(""));

    String input_path = str_lit("inputs/day_01.txt");
    String input = os_data_from_file_path(arena, input_path);
    if (input.size == 0) {
        print("Error: Could not read %.*s\n", (int)input_path.size, input_path.str);
        return;
    }

    print("=== Day 1 ===\n");
    solve_part1(arena, input, 0);
    solve_part1(arena, input, 1);

    print("\n");
    solve_part2(arena, input, Run_Scalar_Slow);
    solve_part2(arena, input, Run_Scalar_Fast); 
    solve_part2(arena, input, Run_Simd);

    arena_release(arena);
}
