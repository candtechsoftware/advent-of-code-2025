#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

static void 
entry_point(Cmd_Line *cmd_line) {
  Arena *arena = arena_alloc();
  log_init(arena, str_lit("meta"));

  String day_str = cmd_line_string(cmd_line, str_lit("day"));
  if (day_str.size == 0) {
    print("Usage:   ./build/meta -day=<N>\n");
    print("Example: ./build/meta -day=1\n");
    return;
  }

  u64 day_num = u64_from_str(day_str, 10);
  if (day_num < 1 || day_num > 12) {
    print("Error: Day must be between 1 and 12\n");
    return;
  }

  String day_padded = str_pushf(arena, "%02llu", day_num);
  String file_path = str_pushf(arena, "src/puzzles/day_%.*s.c",
                               (int)day_padded.size, day_padded.str);

  if (os_file_path_exists(file_path)) {
    print("File already exists: {S}\n", file_path);
    return;
  }

  String template = str_pushf(
      arena,
      "// Advent of Code 2026 - Day %llu\n"
      "\n"
      "#include \"base/base_inc.h\"\n"
      "#include \"os/os_inc.h\"\n"
      "\n"
      "#include \"base/base_inc.c\"\n"
      "#include \"os/os_inc.c\"\n"
      "\n"
      "void\n"
      "solve_part1(Arena *arena, String input) {\n"
      "    // @TODO(Alex): Implement part 1\n"
      "    print(\"Part 1: %{s}\\n\", \"not implemented\");\n"
      "}\n"
      "\n"
      "void\n"
      "solve_part2(Arena *arena, String input) {\n"
      "    // @TODO(Alex): Implement part 2\n"
      "    print(\"Part 2: %{s}\\n\", \"not implemented\");\n"
      "}\n"
      "\n"
      "void\n"
      "entry_point(Cmd_Line *cmd_line) {\n"
      "    Arena *arena = arena_alloc();\n"
      "    log_init(arena, str_lit(\"\"));\n"
      "\n"
      "    // Read input file\n"
      "    String input_path = str_lit(\"inputs/day_%.*s.txt\");\n"
      "    String input = os_data_from_file_path(arena, input_path);\n"
      "    if (input.size == 0) {\n"
      "        print(\"Error: Could not read %%.*s\\n\", (int)input_path.size, "
      "input_path.str);\n"
      "        return;\n"
      "    }\n"
      "\n"
      "    print(\"=== Day %llu ===\\n\");\n"
      "    solve_part1(arena, input);\n"
      "    solve_part2(arena, input);\n"
      "\n"
      "    arena_release(arena);\n"
      "}\n",
      day_num, (int)day_padded.size, day_padded.str, day_num);

  b32 success = os_write_data_to_file(file_path, template);
  if (success) {
    print("Created: {S}\n", file_path);
  } else {
    print("Error: Failed to write %.*s\n", (int)file_path.size, file_path.str);
  }

  arena_release(arena);
}
