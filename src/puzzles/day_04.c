// Advent of Code 2026 - Day 4

#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

void
solve_part1(Arena *arena, String input) {
    // @TODO(Alex): Implement part 1
    print("Part 1: {s}\n", "not implemented");
}

void
solve_part2(Arena *arena, String input) {
    // @TODO(Alex): Implement part 2
    print("Part 2: {s}\n", "not implemented");
}

void
entry_point(Cmd_Line *cmd_line) {
    Arena *arena = arena_alloc();
    log_init(arena, str_lit(""));

    // Read input file
    String input_path = str_lit("inputs/day_04.txt");
    String input = os_data_from_file_path(arena, input_path);
    if (input.size == 0) {
        print("Error: Could not read %.*s\n", (int)input_path.size, input_path.str);
        return;
    }

    print("=== Day 4 ===\n");
    solve_part1(arena, input);
    solve_part2(arena, input);

    arena_release(arena);
}
