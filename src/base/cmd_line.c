static Cmd_Line_Opt **
cmd_line_slot_from_string(Cmd_Line *cmd_line, String string) {
    Cmd_Line_Opt **slot = 0;
    if (cmd_line->option_table_size != 0) {
        u64 hash = u64_hash_from_str(string);
        u64 bucket = hash % cmd_line->option_table_size;
        slot = &cmd_line->option_table[bucket];
    }
    return slot;
}

static Cmd_Line_Opt *
cmd_line_opt_from_slot(Cmd_Line_Opt **slot, String string) {
    Cmd_Line_Opt *result = 0;
    for (Cmd_Line_Opt *var = *slot; var; var = var->hash_next) {
        if (str_match(string, var->string, 0)) {
            result = var;
            break;
        }
    }
    return result;
}

static void
cmd_line_push_opt(Cmd_Line_Opt_List *list, Cmd_Line_Opt *var) {
    SLLQueuePush(list->first, list->last, var);
    list->count += 1;
}

static Cmd_Line_Opt *
cmd_line_insert_opt(Arena *arena, Cmd_Line *cmd_line, String string, String_List values) {
    Cmd_Line_Opt  *var = 0;
    Cmd_Line_Opt **slot = cmd_line_slot_from_string(cmd_line, string);
    Cmd_Line_Opt  *existing_var = cmd_line_opt_from_slot(slot, string);
    if (existing_var != 0) {
        var = existing_var;
    } else {
        var = push_array(arena, Cmd_Line_Opt, 1);
        var->hash_next = *slot;
        var->hash = u64_hash_from_str(string);
        var->string = str_push_copy(arena, string);
        var->value_strings = values;
        String_Join join = {0};
        join.pre = str_lit("");
        join.mid = str_lit(",");
        join.post = str_lit("");
        var->value_string = str_list_join(arena, &var->value_strings, &join);
        *slot = var;
        cmd_line_push_opt(&cmd_line->options, var);
    }
    return var;
}

static Cmd_Line
cmd_line_from_string_list(Arena *arena, String_List command_line) {
    Cmd_Line parsed = {0};
    parsed.exe_name = command_line.first->string;
    parsed.option_table_size = 64;
    parsed.option_table = push_array(arena, Cmd_Line_Opt *, parsed.option_table_size);

    b32 after_passthrough_option = 0;
    b32 first_passthrough = 1;
    for (String_Node *node = command_line.first->next, *next = 0; node != 0; node = next) {
        next = node->next;

        b32    is_option = 0;
        String option_name = node->string;
        if (!after_passthrough_option) {
            is_option = 1;
            if (str_match(node->string, str_lit("--"), 0)) {
                after_passthrough_option = 1;
                is_option = 0;
            } else if (str_match(str_prefix(node->string, 2), str_lit("--"), 0)) {
                option_name = str_skip(option_name, 2);
            } else if (str_match(str_prefix(node->string, 1), str_lit("-"), 0)) {
                option_name = str_skip(option_name, 1);
            } else if (Operating_System_Current == Operating_System_Windows &&
                       str_match(str_prefix(node->string, 1), str_lit("/"), 0)) {
                option_name = str_skip(option_name, 1);
            } else {
                is_option = 0;
            }
        }

        if (is_option) {
            b32    has_values = 0;
            u64    value_signifier_position1 = str_find_needle(option_name, 0, str_lit(":"), 0);
            u64    value_signifier_position2 = str_find_needle(option_name, 0, str_lit("="), 0);
            u64    value_signifier_position = Min(value_signifier_position1, value_signifier_position2);
            String value_portion_this_string = str_skip(option_name, value_signifier_position + 1);
            if (value_signifier_position < option_name.size) {
                has_values = 1;
            }
            option_name = str_prefix(option_name, value_signifier_position);

            String_List values = {0};
            if (has_values) {
                for (String_Node *n = node; n; n = n->next) {
                    next = n->next;
                    String string = n->string;
                    if (n == node) {
                        string = value_portion_this_string;
                    }
                    u8          splits[] = {','};
                    String_List values_in_this_string = str_split(arena, string, splits, ArrayCount(splits));
                    for (String_Node *sub_val = values_in_this_string.first; sub_val; sub_val = sub_val->next) {
                        str_list_push(arena, &values, sub_val->string);
                    }
                    if (!str_match(str_postfix(n->string, 1), str_lit(","), 0) &&
                        (n != node || value_portion_this_string.size != 0)) {
                        break;
                    }
                }
            }

            cmd_line_insert_opt(arena, &parsed, option_name, values);
        }

        else if (!str_match(node->string, str_lit("--"), 0) || !first_passthrough) {
            str_list_push(arena, &parsed.inputs, node->string);
            first_passthrough = 0;
        }
    }

    parsed.argc = command_line.count;
    parsed.argv = push_array(arena, char *, parsed.argc);
    {
        u64 idx = 0;
        for (String_Node *n = command_line.first; n != 0; n = n->next) {
            parsed.argv[idx] = (char *)str_push_copy(arena, n->string).str;
            idx += 1;
        }
    }

    return parsed;
}

static Cmd_Line_Opt *
cmd_line_opt_from_string(Cmd_Line *cmd_line, String name) {
    return cmd_line_opt_from_slot(cmd_line_slot_from_string(cmd_line, name), name);
}

static String_List
cmd_line_strings(Cmd_Line *cmd_line, String name) {
    String_List   result = {0};
    Cmd_Line_Opt *var = cmd_line_opt_from_string(cmd_line, name);
    if (var != 0) {
        result = var->value_strings;
    }
    return result;
}

static String
cmd_line_string(Cmd_Line *cmd_line, String name) {
    String result = {0};

    Cmd_Line_Opt *var = cmd_line_opt_from_string(cmd_line, name);
    if (var != 0) {
        result = var->value_string;
    }
    return result;
}

static b32
cmd_line_has_flag(Cmd_Line *cmd_line, String name) {
    Cmd_Line_Opt *var = cmd_line_opt_from_string(cmd_line, name);
    return (var != 0);
}

static b32
cmd_line_has_argument(Cmd_Line *cmd_line, String name) {
    Cmd_Line_Opt *var = cmd_line_opt_from_string(cmd_line, name);
    return (var != 0 && var->value_strings.count > 0);
}
