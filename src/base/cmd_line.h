#pragma once

typedef struct Cmd_Line_Opt Cmd_Line_Opt;
struct Cmd_Line_Opt {
    Cmd_Line_Opt *next;
    Cmd_Line_Opt *hash_next;
    u64           hash;
    String        string;
    String_List   value_strings;
    String        value_string;
};

typedef struct Cmd_Line_Opt_List Cmd_Line_Opt_List;
struct Cmd_Line_Opt_List {
    u64           count;
    Cmd_Line_Opt *first;
    Cmd_Line_Opt *last;
};

typedef struct Cmd_Line Cmd_Line;
struct Cmd_Line {
    String            exe_name;
    Cmd_Line_Opt_List options;
    String_List       inputs;
    u64               option_table_size;
    Cmd_Line_Opt    **option_table;
    u64               argc;
    char            **argv;
};

static Cmd_Line_Opt **cmd_line_slot_from_string(Cmd_Line *cmd_line, String string);
static Cmd_Line_Opt  *cmd_line_opt_from_slot(Cmd_Line_Opt **slot, String string);
static void           cmd_line_push_opt(Cmd_Line_Opt_List *list, Cmd_Line_Opt *var);
static Cmd_Line_Opt  *cmd_line_insert_opt(Arena *arena, Cmd_Line *cmd_line, String string, String_List values);
static Cmd_Line       cmd_line_from_string_list(Arena *arena, String_List command_line);
static Cmd_Line_Opt  *cmd_line_opt_from_string(Cmd_Line *cmd_line, String name);
static String_List    cmd_line_strings(Cmd_Line *cmd_line, String name);
static String         cmd_line_string(Cmd_Line *cmd_line, String name);
static b32            cmd_line_has_flag(Cmd_Line *cmd_line, String name);
static b32            cmd_line_has_argument(Cmd_Line *cmd_line, String name);
