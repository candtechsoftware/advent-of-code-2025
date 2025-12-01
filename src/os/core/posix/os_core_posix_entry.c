typedef struct Cmd_Line Cmd_Line;

static void entry_point(Cmd_Line *cmd_line);

static void
posix_signal_handler(int sig, siginfo_t *info, void *arg) {
    const char *sig_name = "UNKNOWN";
    switch (sig) {
		case SIGILL:
        sig_name = "SIGILL (Illegal instruction)";
        break;
		case SIGTRAP:
        sig_name = "SIGTRAP (Trap)";
        break;
		case SIGABRT:
        sig_name = "SIGABRT (Abort)";
        break;
		case SIGFPE:
        sig_name = "SIGFPE (Floating point exception)";
        break;
		case SIGBUS:
        sig_name = "SIGBUS (Bus error)";
        break;
		case SIGSEGV:
        sig_name = "SIGSEGV (Segmentation fault)";
        break;
		case SIGQUIT:
        sig_name = "SIGQUIT (Quit)";
        break;
    }
	
    fprintf(stderr, "\n=== FATAL SIGNAL ===\n");
    fprintf(stderr, "Signal: %s (%d)\n", sig_name, sig);
	
    if (info) {
        fprintf(stderr, "Fault address: %p\n", info->si_addr);
        fprintf(stderr, "Signal code: %d\n", info->si_code);
    }
	
    signal(sig, SIG_DFL);
    raise(sig);
}

int main(int argc, char **argv) {
    {
        struct sigaction handler = {
            .sa_sigaction = posix_signal_handler,
            .sa_flags = SA_SIGINFO,
        };
        sigfillset(&handler.sa_mask);
        sigaction(SIGILL, &handler, NULL);
        sigaction(SIGTRAP, &handler, NULL);
        sigaction(SIGABRT, &handler, NULL);
        sigaction(SIGFPE, &handler, NULL);
        sigaction(SIGBUS, &handler, NULL);
        sigaction(SIGSEGV, &handler, NULL);
        sigaction(SIGQUIT, &handler, NULL);
    }
	
    {
        {
            OS_System_Info *info = &os_posix_state.system_info;
#if OS_LINUX
            info->logical_processors = (u32)get_nprocs();
#elif OS_MAC
            int    count = 0;
            size_t count_len = sizeof(count);
            sysctlbyname("hw.logicalcpu", &count, &count_len, NULL, 0);
            info->logical_processors = (u32)count;
#endif
            info->page_size = (u64)getpagesize();
        }
        {
            OS_Process_Info *info = &os_posix_state.process_info;
            info->pid = (u32)getpid();
        }
		
        os_posix_state.arena = arena_alloc();
        os_posix_state.entity_arena = arena_alloc();
        pthread_mutex_init(&os_posix_state.entity_mutex, 0);
		
        {
            TCTX *main_tctx = tctx_alloc();
            tctx_select(main_tctx);
            tctx_set_thread_name(str_lit("Main Thread"));
        }
		
        {
            Scratch          scratch = arena_get_scratch(0, 0);
            OS_Process_Info *info = &os_posix_state.process_info;
			
            {
                b32 got_final_result = 0;
                u8 *buffer = 0;
                u64 size = 0;
#if OS_LINUX
                for (s64 cap = PATH_MAX, r = 0; r < 4; cap *= 2, r += 1) {
                    arena_end_scratch(&scratch);
                    buffer = push_array_no_zero(scratch.arena, u8, cap);
                    ssize_t result = readlink("/proc/self/exe", (char *)buffer, cap);
                    if (result >= 0 && result < cap) {
                        size = result;
                        got_final_result = 1;
                        break;
                    }
                }
#elif OS_MAC
                for (s64 cap = MAXPATHLEN, r = 0; r < 4; cap *= 2, r += 1) {
                    arena_end_scratch(&scratch);
                    buffer = push_array_no_zero(scratch.arena, u8, cap);
                    uint32_t bufsize = cap;
                    if (_NSGetExecutablePath((char *)buffer, &bufsize) == 0) {
                        size = strlen((char *)buffer);
                        got_final_result = 1;
                        break;
                    }
                }
#endif
				
                if (got_final_result && size > 0) {
                    String full_name = {buffer, size};
                    String name_chopped = str_chop_last_slash(full_name);
                    info->binary_path = str_push_copy(os_posix_state.arena, name_chopped);
                }
            }
			
            {
                info->initial_path = os_get_current_path(os_posix_state.arena);
            }
			
            {
                char *home = getenv("HOME");
                if (home) {
                    info->user_program_data_path = str_cstring((u8 *)home);
                }
            }
			
            arena_end_scratch(&scratch);
        }
    }
	
    Scratch     scratch = arena_get_scratch(0, 0);
    String_List args_list = {0};
    for (int i = 0; i < argc; i++) {
        str_list_push(scratch.arena, &args_list, str_cstring((u8 *)argv[i]));
    }
    Cmd_Line cmd_line = cmd_line_from_string_list(scratch.arena, args_list);
	
    entry_point(&cmd_line);
	
    arena_end_scratch(&scratch);
	
    return 0;
}
