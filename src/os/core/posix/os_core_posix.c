#include "os_core_posix.h"

OS_Handle os_handle_zero(void) {
    OS_Handle result = {0};
    return result;
}

b32 os_handle_match(OS_Handle a, OS_Handle b) {
    return a.v[0] == b.v[0];
}

void os_handle_list_push(Arena *arena, OS_Handle_List *handles, OS_Handle handle) {
    OS_Handle_Node *node = push_array(arena, OS_Handle_Node, 1);
    node->v = handle;
    SLLQueuePush(handles->first, handles->last, node);
    handles->count += 1;
}

OS_Handle_Array os_handle_array_from_list(Arena *arena, OS_Handle_List list) {
    OS_Handle_Array result = {0};
    result.v = push_array(arena, OS_Handle, list.count);
    result.count = list.count;
    u64 idx = 0;
    for (OS_Handle_Node *n = list.first; n != 0; n = n->next) {
        result.v[idx++] = n->v;
    }
    return result;
}

String os_string_from_file_range(Arena *arena, OS_Handle file, Rng1_u64 range) {
    String result = {0};
    u64    size_to_read = range.max - range.min;
    u8    *buffer = push_array(arena, u8, size_to_read + 1);
    u64    actual_size = os_file_read(file, range, buffer);
    buffer[actual_size] = 0;
    result.str = buffer;
    result.size = actual_size;
    return result;
}

String os_data_from_file_path(Arena *arena, String path) {
    OS_Handle       file = os_file_open(OS_Access_Flag_Read | OS_Access_Flag_Share_Read, path);
    File_Properties props = os_properties_from_file(file);
    Rng1_u64        range = {0, props.size};
    String          data = os_string_from_file_range(arena, file, range);
    os_file_close(file);
    return data;
}

b32 os_write_data_to_file(String path, String data) {
    OS_Handle file = os_file_open(OS_Access_Flag_Write, path);
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    Rng1_u64 rng = {0, data.size};
    u64      written = os_file_write(file, rng, data.str);
    os_file_close(file);
    return (written == data.size);
}

b32 os_write_data_list_to_file_path(String path, String_List list) {
    OS_Handle file = os_file_open(OS_Access_Flag_Write, path);
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    u64 offset = 0;
    b32 success = 1;
    for (String_Node *n = list.first; n != 0; n = n->next) {
        Rng1_u64 rng = {offset, offset + n->string.size};
        u64      written = os_file_write(file, rng, n->string.str);
        if (written != n->string.size) {
            success = 0;
            break;
        }
        offset += n->string.size;
    }
    os_file_close(file);
    return success;
}

b32 os_append_data_to_file_path(String path, String data) {
    OS_Handle file = os_file_open(OS_Access_Flag_Write | OS_Access_Flag_Append, path);
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    File_Properties props = os_properties_from_file(file);
    Rng1_u64        rng = {props.size, props.size + data.size};
    u64             written = os_file_write(file, rng, data.str);
    os_file_close(file);
    return (written == data.size);
}

OS_File_ID os_id_from_file_path(String path) {
    OS_Handle  file = os_file_open(OS_Access_Flag_Read, path);
    OS_File_ID id = os_id_from_file(file);
    os_file_close(file);
    return id;
}

s64 os_file_id_compare(OS_File_ID a, OS_File_ID b) {
    if (a.v[0] != b.v[0])
        return (s64)(a.v[0] - b.v[0]);
    if (a.v[1] != b.v[1])
        return (s64)(a.v[1] - b.v[1]);
    if (a.v[2] != b.v[2])
        return (s64)(a.v[2] - b.v[2]);
    return 0;
}

String os_file_read_cstring(Arena *arena, OS_Handle file, u64 off) {
    u64 chunk_size = 256;
    u64 total_size = 0;
    u8 *buffer = 0;

    for (;;) {
        u64 new_size = total_size + chunk_size;
        u8 *new_buffer = push_array(arena, u8, new_size + 1);
        if (buffer) {
            MemoryCopy(new_buffer, buffer, total_size);
        }
        buffer = new_buffer;

        Rng1_u64 rng = {off + total_size, off + new_size};
        u64      bytes_read = os_file_read(file, rng, buffer + total_size);

        for (u64 i = 0; i < bytes_read; i++) {
            if (buffer[total_size + i] == 0) {
                String result = {buffer, total_size + i};
                return result;
            }
        }

        total_size += bytes_read;
        if (bytes_read < chunk_size) {
            break;
        }
    }

    buffer[total_size] = 0;
    String result = {buffer, total_size};
    return result;
}

String_List os_string_from_argcv(Arena *arena, int argc, char **argv) {
    String_List result = {0};
    for (int i = 0; i < argc; i++) {
        str_list_push(arena, &result, str_cstring((u8 *)argv[i]));
    }
    return result;
}

static Date_Time os_posix_date_time_from_tm(tm in, u32 msec) {
    Date_Time dt = {0};
    dt.sec = in.tm_sec;
    dt.min = in.tm_min;
    dt.hour = in.tm_hour;
    dt.day = in.tm_mday - 1;
    dt.mon = in.tm_mon;
    dt.year = in.tm_year + 1900;
    dt.msec = msec;
    return dt;
}

static tm os_posix_tm_from_date_time(Date_Time dt) {
    tm result = {0};
    result.tm_sec = dt.sec;
    result.tm_min = dt.min;
    result.tm_hour = dt.hour;
    result.tm_mday = dt.day + 1;
    result.tm_mon = dt.mon;
    result.tm_year = dt.year - 1900;
    return result;
}

static timespec os_posix_timespec_from_date_time(Date_Time dt) {
    tm       tm_val = os_posix_tm_from_date_time(dt);
    time_t   seconds = timegm(&tm_val);
    timespec result = {0};
    result.tv_sec = seconds;
    return result;
}

static Dense_Time os_posix_dense_time_from_timespec(timespec in) {
    Dense_Time result = 0;
    {
        struct tm tm_time = {0};
        gmtime_r(&in.tv_sec, &tm_time);
        Date_Time date_time = os_posix_date_time_from_tm(tm_time, in.tv_nsec / Million(1));
        result = dense_time_from_date_time(date_time);
    }
    return result;
}

static File_Properties os_posix_file_properties_from_stat(struct stat *s) {
    File_Properties props = {0};
    props.size = s->st_size;
#if OS_MAC
    props.create_time = os_posix_dense_time_from_timespec(s->st_ctimespec);
    props.modify_time = os_posix_dense_time_from_timespec(s->st_mtimespec);
#elif OS_LINUX
    props.create_time = os_posix_dense_time_from_timespec(s->st_ctim);
    props.modify_time = os_posix_dense_time_from_timespec(s->st_mtim);
#endif
    if (s->st_mode & S_IFDIR) {
        props.flags |= File_Property_Flag_Is_Folder;
    }
    return props;
}

static void os_posix_safe_call_sig_handler(int x) {
    OS_Posix_Safe_Call_Chain *chain = os_posix_safe_call_chain;
    if (chain != 0 && chain->fail_handler != 0) {
        chain->fail_handler(chain->ptr);
    }
    abort();
}

static OS_Posix_Entity *os_posix_entity_alloc(OS_Posix_Entity_Kind kind) {
    OS_Posix_Entity *entity = 0;
    pthread_mutex_lock(&os_posix_state.entity_mutex);
    {
        entity = os_posix_state.entity_free;
        if (entity) {
            SLLStackPop(os_posix_state.entity_free);
        } else {
            entity = push_array_no_zero(os_posix_state.entity_arena, OS_Posix_Entity, 1);
        }
    }
    pthread_mutex_unlock(&os_posix_state.entity_mutex);
    MemoryZeroStruct(entity);
    entity->kind = kind;
    return entity;
}

static void os_posix_entity_release(OS_Posix_Entity *entity) {
    pthread_mutex_lock(&os_posix_state.entity_mutex);
    {
        SLLStackPush(os_posix_state.entity_free, entity);
    }
    pthread_mutex_unlock(&os_posix_state.entity_mutex);
}

static void *os_posix_thread_entry_point(void *ptr) {
    TCTX *tctx = tctx_alloc();
    tctx_select(tctx);

    OS_Posix_Entity    *entity = (OS_Posix_Entity *)ptr;
    Thread_Entry_Point *func = entity->thread.func;
    void               *thread_ptr = entity->thread.ptr;

    func(thread_ptr);

    tctx_release(tctx);

    return 0;
}

OS_System_Info *os_get_system_info(void) {
    return &os_posix_state.system_info;
}

OS_Process_Info *os_get_process_info(void) {
    return &os_posix_state.process_info;
}

String os_get_current_path(Arena *arena) {
    char  *cwdir = getcwd(0, 0);
    String string = str_push_copy(arena, str_cstring((u8 *)cwdir));
    free(cwdir);
    return string;
}

u32 os_get_process_start_time_unix(void) {
    Scratch scratch = arena_get_scratch(0, 0);
    u64     start_time = 0;
#if OS_LINUX
    pid_t       pid = getpid();
    String      path = str_pushf(scratch.arena, "/proc/%u", pid);
    struct stat st;
    int         err = stat((char *)path.str, &st);
    if (err == 0) {
        start_time = st.st_mtime;
    }
#elif OS_MAC
    start_time = (u32)time(NULL);
#endif
    arena_end_scratch(&scratch);
    return (u32)start_time;
}

void *os_reserve(u64 size) {
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
        result = 0;
    }
    return result;
}

b32 os_commit(void *ptr, u64 size) {
    if (!ptr || size == 0) {
        return 0;
    }

    u64 page_size = (u64)getpagesize();

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t page_aligned_addr = addr & ~(page_size - 1);
    u64       offset = addr - page_aligned_addr;
    u64       page_aligned_size = AlignUpPow2(size + offset, page_size);

    int result = mprotect((void *)page_aligned_addr, page_aligned_size, PROT_READ | PROT_WRITE);
    return (result == 0);
}

void os_decommit(void *ptr, u64 size) {
    madvise(ptr, size, MADV_DONTNEED);
    mprotect(ptr, size, PROT_NONE);
}

void os_mem_release(void *ptr, u64 size) {
    munmap(ptr, size);
}

void *os_reserve_large(u64 size) {
#if OS_LINUX
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (result == MAP_FAILED) {
        result = 0;
    }
    return result;
#else
    return os_reserve(size);
#endif
}

b32 os_commit_large(void *ptr, u64 size) {
    mprotect(ptr, size, PROT_READ | PROT_WRITE);
    return 1;
}

u32 os_tid(void) {
#if OS_LINUX
    u32 result = gettid();
#elif OS_MAC
    u64 tid;
    pthread_threadid_np(NULL, &tid);
    u32 result = (u32)tid;
#endif
    return result;
}

void os_set_thread_name(String name) {
    Scratch   scratch = arena_get_scratch(0, 0);
    String    name_copy = str_push_copy(scratch.arena, name);
    pthread_t current_thread = pthread_self();
#if OS_LINUX
    pthread_setname_np(current_thread, (char *)name_copy.str);
#elif OS_MAC
    pthread_setname_np((char *)name_copy.str);
#endif
    arena_end_scratch(&scratch);
}

void os_abort(s32 exit_code) {
    exit(exit_code);
}

OS_Handle os_file_open(OS_Access_Flags flags, String path) {
    Scratch scratch = arena_get_scratch(0, 0);
    u8     *path_cstr = push_array(scratch.arena, u8, path.size + 1);
    MemoryCopy(path_cstr, path.str, path.size);
    path_cstr[path.size] = 0;
    int posix_flags = 0;
    if (flags & OS_Access_Flag_Read && flags & OS_Access_Flag_Write) {
        posix_flags = O_RDWR;
    } else if (flags & OS_Access_Flag_Write) {
        posix_flags = O_WRONLY;
    } else if (flags & OS_Access_Flag_Read) {
        posix_flags = O_RDONLY;
    }
    if (flags & OS_Access_Flag_Append) {
        posix_flags |= O_APPEND;
    }
    if (flags & (OS_Access_Flag_Write | OS_Access_Flag_Append)) {
        posix_flags |= O_CREAT;
    }
    posix_flags |= O_CLOEXEC;
    int       fd = open((char *)path_cstr, posix_flags, 0755);
    OS_Handle handle = {0};
    if (fd != -1) {
        handle.v[0] = fd;
    }
    arena_end_scratch(&scratch);
    return handle;
}

void os_file_close(OS_Handle file) {
    if (os_handle_match(file, os_handle_zero())) {
        return;
    }
    int fd = (int)file.v[0];
    close(fd);
}

u64 os_file_read(OS_Handle file, Rng1_u64 rng, void *out_data) {
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    int fd = (int)file.v[0];
    u64 total_num_bytes_to_read = rng.max - rng.min;
    u64 total_num_bytes_read = 0;
    u64 total_num_bytes_left_to_read = total_num_bytes_to_read;
    for (; total_num_bytes_left_to_read > 0;) {
        int read_result = pread(fd, (u8 *)out_data + total_num_bytes_read, total_num_bytes_left_to_read, rng.min + total_num_bytes_read);
        if (read_result >= 0) {
            total_num_bytes_read += read_result;
            total_num_bytes_left_to_read -= read_result;
        } else if (errno != EINTR) {
            break;
        }
    }
    return total_num_bytes_read;
}

u64 os_file_write(OS_Handle file, Rng1_u64 rng, void *data) {
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    int fd = (int)file.v[0];
    u64 total_num_bytes_to_write = rng.max - rng.min;
    u64 total_num_bytes_written = 0;
    u64 total_num_bytes_left_to_write = total_num_bytes_to_write;
    for (; total_num_bytes_left_to_write > 0;) {
        int write_result = pwrite(fd, (u8 *)data + total_num_bytes_written, total_num_bytes_left_to_write, rng.min + total_num_bytes_written);
        if (write_result >= 0) {
            total_num_bytes_written += write_result;
            total_num_bytes_left_to_write -= write_result;
        } else if (errno != EINTR) {
            break;
        }
    }
    return total_num_bytes_written;
}

b32 os_file_set_times(OS_Handle file, Date_Time date_time) {
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    int      fd = (int)file.v[0];
    timespec time = os_posix_timespec_from_date_time(date_time);
    timespec times[2] = {time, time};
    int      futimens_result = futimens(fd, times);
    b32      good = (futimens_result != -1);
    return good;
}

File_Properties os_properties_from_file(OS_Handle file) {
    if (os_handle_match(file, os_handle_zero())) {
        return (File_Properties){0};
    }
    int             fd = (int)file.v[0];
    struct stat     fd_stat = {0};
    int             fstat_result = fstat(fd, &fd_stat);
    File_Properties props = {0};
    if (fstat_result != -1) {
        props = os_posix_file_properties_from_stat(&fd_stat);
    }
    return props;
}

OS_File_ID os_id_from_file(OS_Handle file) {
    if (os_handle_match(file, os_handle_zero())) {
        return (OS_File_ID){0};
    }
    int         fd = (int)file.v[0];
    struct stat fd_stat = {0};
    int         fstat_result = fstat(fd, &fd_stat);
    OS_File_ID  id = {0};
    if (fstat_result != -1) {
        id.v[0] = fd_stat.st_dev;
        id.v[1] = fd_stat.st_ino;
    }
    return id;
}

b32 os_file_reserve_size(OS_Handle file, u64 size) {
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    int fd = (int)file.v[0];
    int result = ftruncate(fd, (off_t)size);
    return (result == 0);
}

b32 os_delete_file_at_path(String path) {
    Scratch scratch = arena_get_scratch(0, 0);
    b32     result = 0;
    String  path_copy = str_push_copy(scratch.arena, path);
    if (remove((char *)path_copy.str) != -1) {
        result = 1;
    }
    arena_end_scratch(&scratch);
    return result;
}

b32 os_copy_file_path(String dst, String src) {
    b32 result = 0;
#if OS_MAC
    Scratch scratch = arena_get_scratch(0, 0);
    String  src_copy = str_push_copy(scratch.arena, src);
    String  dst_copy = str_push_copy(scratch.arena, dst);
    int     copy_result = copyfile((char *)src_copy.str, (char *)dst_copy.str, NULL, COPYFILE_DATA);
    result = (copy_result == 0);
    arena_end_scratch(&scratch);
#elif OS_LINUX
    OS_Handle src_h = os_file_open(OS_Access_Flag_Read, src);
    OS_Handle dst_h = os_file_open(OS_Access_Flag_Write, dst);
    if (!os_handle_match(src_h, os_handle_zero()) &&
        !os_handle_match(dst_h, os_handle_zero())) {
        int             src_fd = (int)src_h.v[0];
        int             dst_fd = (int)dst_h.v[0];
        File_Properties src_props = os_properties_from_file(src_h);
        u64             size = src_props.size;
        u64             total_bytes_copied = 0;
        u64             bytes_left_to_copy = size;
        for (; bytes_left_to_copy > 0;) {
            off_t sendfile_off = total_bytes_copied;
            int   send_result = sendfile(dst_fd, src_fd, &sendfile_off, bytes_left_to_copy);
            if (send_result <= 0) {
                break;
            }
            u64 bytes_copied = (u64)send_result;
            bytes_left_to_copy -= bytes_copied;
            total_bytes_copied += bytes_copied;
        }
        result = (total_bytes_copied == size);
    }
    os_file_close(src_h);
    os_file_close(dst_h);
#endif
    return result;
}

b32 os_move_file_path(String dst, String src) {
    b32     good = 0;
    Scratch scratch = arena_get_scratch(0, 0);
    {
        char *src_cstr = (char *)str_push_copy(scratch.arena, src).str;
        char *dst_cstr = (char *)str_push_copy(scratch.arena, dst).str;
        int   rename_result = rename(src_cstr, dst_cstr);
        good = (rename_result != -1);
    }
    arena_end_scratch(&scratch);
    return good;
}

String os_full_path_from_path(Arena *arena, String path) {
    Scratch scratch = arena_get_scratch(&arena, 1);
    String  path_copy = str_push_copy(scratch.arena, path);
#if OS_MAC
    char buffer[MAXPATHLEN] = {0};
#else
    char buffer[PATH_MAX] = {0};
#endif
    realpath((char *)path_copy.str, buffer);
    String result = str_push_copy(arena, str_cstring((u8 *)buffer));
    arena_end_scratch(&scratch);
    return result;
}

b32 os_file_path_exists(String path) {
    Scratch scratch = arena_get_scratch(0, 0);
    u8     *path_cstr = push_array(scratch.arena, u8, path.size + 1);
    MemoryCopy(path_cstr, path.str, path.size);
    path_cstr[path.size] = 0;
    int access_result = access((char *)path_cstr, F_OK);
    b32 result = 0;
    if (access_result == 0) {
        result = 1;
    }
    arena_end_scratch(&scratch);
    return result;
}

b32 os_folder_path_exists(String path) {
    Scratch scratch = arena_get_scratch(0, 0);
    b32     exists = 0;
    u8     *path_cstr = push_array(scratch.arena, u8, path.size + 1);
    MemoryCopy(path_cstr, path.str, path.size);
    path_cstr[path.size] = 0;
    DIR *handle = opendir((char *)path_cstr);
    if (handle) {
        closedir(handle);
        exists = 1;
    }
    arena_end_scratch(&scratch);
    return exists;
}

File_Properties os_properties_from_file_path(String path) {
    Scratch         scratch = arena_get_scratch(0, 0);
    String          path_copy = str_push_copy(scratch.arena, path);
    struct stat     f_stat = {0};
    int             stat_result = stat((char *)path_copy.str, &f_stat);
    File_Properties props = {0};
    if (stat_result != -1) {
        props = os_posix_file_properties_from_stat(&f_stat);
    }
    arena_end_scratch(&scratch);
    return props;
}

OS_Handle os_file_map_open(OS_Access_Flags flags, OS_Handle file) {
    OS_Handle map = file;
    return map;
}

void os_file_map_close(OS_Handle map) {
    // NOTE: nothing to do; `map` handles are the same as `file` handles in
    // the POSIX implementation (on Windows they require separate handles)
}

void *os_file_map_view_open(OS_Handle map, OS_Access_Flags flags, Rng1_u64 range) {
    if (os_handle_match(map, os_handle_zero())) {
        return 0;
    }
    int fd = (int)map.v[0];
    int prot_flags = 0;
    if (flags & OS_Access_Flag_Write) {
        prot_flags |= PROT_WRITE;
    }
    if (flags & OS_Access_Flag_Read) {
        prot_flags |= PROT_READ;
    }
    int   map_flags = MAP_PRIVATE;
    void *base = mmap(0, range.max - range.min, prot_flags, map_flags, fd, range.min);
    if (base == MAP_FAILED) {
        base = 0;
    }
    return base;
}

void os_file_map_view_close(OS_Handle map, void *ptr, Rng1_u64 range) {
    munmap(ptr, range.max - range.min);
}

OS_File_Iter *os_file_iter_begin(Arena *arena, String path, OS_Filter_Iter_Flags flags) {
    OS_File_Iter *base_iter = push_array(arena, OS_File_Iter, 1);
    base_iter->flags = flags;
    OS_Posix_File_Iter *iter = (OS_Posix_File_Iter *)base_iter->memory;
    {
        String path_copy = str_push_copy(arena, path);
        iter->dir = opendir((char *)path_copy.str);
        iter->path = path_copy;
    }
    return base_iter;
}

b32 os_file_iter_next(Arena *arena, OS_File_Iter *iter, File_Properties *info_out) {
    b32                 good = 0;
    OS_Posix_File_Iter *posix_iter = (OS_Posix_File_Iter *)iter->memory;
    for (; posix_iter->dir != 0;) {
        posix_iter->dp = readdir(posix_iter->dir);
        good = (posix_iter->dp != 0);

        struct stat st = {0};
        int         stat_result = 0;
        if (good) {
            Scratch scratch = arena_get_scratch(&arena, 1);
            String  full_path = str_pushf(scratch.arena, "%S/%s", posix_iter->path, posix_iter->dp->d_name);
            String  null_term = str_push_copy(scratch.arena, full_path);
            ((u8 *)null_term.str)[null_term.size] = 0;
            stat_result = stat((char *)null_term.str, &st);
            arena_end_scratch(&scratch);
        }

        b32 filtered = 0;
        if (good) {
            filtered = ((st.st_mode == S_IFDIR && iter->flags & OS_Filter_Iter_Flag_Skip_Folders) ||
                        (st.st_mode == S_IFREG && iter->flags & OS_Filter_Iter_Flag_Skip_Files) ||
                        (posix_iter->dp->d_name[0] == '.' && posix_iter->dp->d_name[1] == 0) ||
                        (posix_iter->dp->d_name[0] == '.' && posix_iter->dp->d_name[1] == '.' && posix_iter->dp->d_name[2] == 0));
        }

        if (good && !filtered) {
            info_out->name = str_push_copy(arena, str_cstring((u8 *)posix_iter->dp->d_name));
            if (stat_result != -1) {
                *info_out = os_posix_file_properties_from_stat(&st);
                info_out->name = str_push_copy(arena, str_cstring((u8 *)posix_iter->dp->d_name));
            }
            break;
        }

        if (!good) {
            break;
        }
    }
    return good;
}

void os_file_iter_end(OS_File_Iter *iter) {
    OS_Posix_File_Iter *posix_iter = (OS_Posix_File_Iter *)iter->memory;
    closedir(posix_iter->dir);
}

b32 os_make_directory(String path) {
    Scratch scratch = arena_get_scratch(0, 0);
    b32     result = 0;
    String  path_copy = str_push_copy(scratch.arena, path);
    if (mkdir((char *)path_copy.str, 0755) != -1) {
        result = 1;
    }
    arena_end_scratch(&scratch);
    return result;
}

OS_Handle os_shared_memory_alloc(u64 size, String name) {
    Scratch scratch = arena_get_scratch(0, 0);
    String  name_copy = str_push_copy(scratch.arena, name);
    int     id = shm_open((char *)name_copy.str, O_RDWR | O_CREAT, 0666);
    ftruncate(id, size);
    OS_Handle result = {(u64)id};
    arena_end_scratch(&scratch);
    return result;
}

OS_Handle os_shared_memory_open(String name) {
    Scratch   scratch = arena_get_scratch(0, 0);
    String    name_copy = str_push_copy(scratch.arena, name);
    int       id = shm_open((char *)name_copy.str, O_RDWR, 0);
    OS_Handle result = {(u64)id};
    arena_end_scratch(&scratch);
    return result;
}

void os_shared_memory_close(OS_Handle handle) {
    if (os_handle_match(handle, os_handle_zero())) {
        return;
    }
    int id = (int)handle.v[0];
    close(id);
}

void *os_shared_memory_view_open(OS_Handle handle, Rng1_u64 range) {
    if (os_handle_match(handle, os_handle_zero())) {
        return 0;
    }
    int   id = (int)handle.v[0];
    void *base = mmap(0, range.max - range.min, PROT_READ | PROT_WRITE, MAP_SHARED, id, range.min);
    if (base == MAP_FAILED) {
        base = 0;
    }
    return base;
}

void os_shared_memory_view_close(OS_Handle handle, void *ptr, Rng1_u64 range) {
    if (os_handle_match(handle, os_handle_zero())) {
        return;
    }
    munmap(ptr, range.max - range.min);
}

u64 os_now_microseconds(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    u64 result = t.tv_sec * Million(1) + (t.tv_nsec / Thousand(1));
    return result;
}

u32 os_now_unix(void) {
    time_t t = time(0);
    return (u32)t;
}

Date_Time os_now_universal_time(void) {
    time_t t = 0;
    time(&t);
    struct tm universal_tm = {0};
    gmtime_r(&t, &universal_tm);
    Date_Time result = os_posix_date_time_from_tm(universal_tm, 0);
    return result;
}

Date_Time os_universal_time_from_local(Date_Time *date_time) {
    tm local_tm = os_posix_tm_from_date_time(*date_time);
    local_tm.tm_isdst = -1;
    time_t universal_t = mktime(&local_tm);

    tm universal_tm = {0};
    gmtime_r(&universal_t, &universal_tm);
    Date_Time result = os_posix_date_time_from_tm(universal_tm, 0);
    return result;
}

Date_Time os_local_time_from_universal(Date_Time *date_time) {
    tm universal_tm = os_posix_tm_from_date_time(*date_time);
    universal_tm.tm_isdst = -1;
    time_t universal_t = timegm(&universal_tm);
    tm     local_tm = {0};
    localtime_r(&universal_t, &local_tm);

    Date_Time result = os_posix_date_time_from_tm(local_tm, 0);
    return result;
}

void os_sleep_milliseconds(u32 msec) {
    usleep(msec * Thousand(1));
}

OS_Handle os_process_launch(OS_Process_Launch_Params *params) {
    OS_Handle handle = {0};

    posix_spawn_file_actions_t file_actions = {0};
    int                        file_actions_init_code = posix_spawn_file_actions_init(&file_actions);
    if (file_actions_init_code == 0) {
        int stdout_code = posix_spawn_file_actions_adddup2(&file_actions, (int)params->stdout_file.v[0], STDOUT_FILENO);
        Assert(stdout_code == 0);

        int stderr_code = posix_spawn_file_actions_adddup2(&file_actions, (int)params->stderr_file.v[0], STDERR_FILENO);
        Assert(stderr_code == 0);

        int stdin_code = posix_spawn_file_actions_adddup2(&file_actions, (int)params->stdin_file.v[0], STDIN_FILENO);
        Assert(stdin_code == 0);

        posix_spawnattr_t attr = {0};
        int               attr_init_code = posix_spawnattr_init(&attr);
        if (attr_init_code == 0) {
            Scratch scratch = arena_get_scratch(0, 0);

            char **argv = push_array(scratch.arena, char *, params->cmd_line.count + 1);
            {
                String path_to_exe = str_pushf(scratch.arena, "%S/%S", params->path, params->cmd_line.first->string);
                argv[0] = (char *)path_to_exe.str;
                u64 arg_idx = 1;
                for (String_Node *n = params->cmd_line.first->next; n != 0; n = n->next) {
                    argv[arg_idx++] = (char *)n->string.str;
                }
            }

            char **envp = 0;
            if (params->inherit_env) {
                extern char **environ;
                envp = environ;
            } else {
                envp = push_array(scratch.arena, char *, params->env.count + 2);
                u64 env_idx = 0;
                for (String_Node *n = params->env.first; n != 0; n = n->next) {
                    envp[env_idx] = (char *)n->string.str;
                }
            }

            if (params->debug_process) {
                InvalidPath;
            }

            if (!params->consoleless) {
                NotImplemented;
            }

            pid_t pid = 0;
            int   spawn_code = posix_spawn(&pid, argv[0], &file_actions, &attr, argv, envp);

            if (spawn_code == 0) {
                handle.v[0] = (u64)pid;
            }

            int attr_destroy_code = posix_spawnattr_destroy(&attr);
            Assert(attr_destroy_code == 0);

            arena_end_scratch(&scratch);
        }

        int file_actions_destroy_code = posix_spawn_file_actions_destroy(&file_actions);
        Assert(file_actions_destroy_code == 0);
    }

    return handle;
}

b32 os_process_join(OS_Handle handle, u64 endt_us, u64 *exit_code_out) {
    pid_t pid = (pid_t)handle.v[0];
    b32   result = 0;
    if (endt_us == 0) {
        if (kill(pid, 0) >= 0) {
            result = (errno == ENOENT);

            if (result) {
                int status;
                waitpid(pid, &status, 0);
            }
        } else {
            Assert(0 && "failed to get status from pid");
        }
    } else if (endt_us == MAX_U64) {
        for (;;) {
            int status = 0;
            int w = waitpid(pid, &status, 0);
            if (w == -1) {
                break;
            }
            if (WIFEXITED(status) || WIFSTOPPED(status) || WIFSIGNALED(status)) {
                result = 1;
                break;
            }
        }
    } else {
        NotImplemented;
    }
    return result;
}

void os_process_detach(OS_Handle handle) {
    // No need to close pid
}

b32 os_process_kill(OS_Handle handle) {
    int error_code = kill((pid_t)handle.v[0], SIGKILL);
    b32 is_killed = error_code == 0;
    return is_killed;
}

Thread os_thread_launch(Thread_Entry_Point *func, void *ptr) {
    OS_Posix_Entity *entity = os_posix_entity_alloc(OS_Posix_Entity_Kind_Thread);
    entity->thread.func = func;
    entity->thread.ptr = ptr;
    {
        int pthread_result = pthread_create(&entity->thread.handle, 0, os_posix_thread_entry_point, entity);
        if (pthread_result == -1) {
            os_posix_entity_release(entity);
            entity = 0;
        }
    }
    Thread handle = {(u64)entity};
    return handle;
}

b32 os_thread_join(Thread handle, u64 endt_us) {
    if (MemoryIsZeroStruct(&handle)) {
        return 0;
    }
    OS_Posix_Entity *entity = (OS_Posix_Entity *)handle.v[0];
    int              join_result = pthread_join(entity->thread.handle, 0);
    b32              result = (join_result == 0);
    os_posix_entity_release(entity);
    return result;
}

void os_thread_detach(Thread handle) {
    if (MemoryIsZeroStruct(&handle)) {
        return;
    }
    OS_Posix_Entity *entity = (OS_Posix_Entity *)handle.v[0];
    os_posix_entity_release(entity);
}

Mutex os_mutex_alloc(void) {
    OS_Posix_Entity *entity = os_posix_entity_alloc(OS_Posix_Entity_Kind_Mutex);
    int              init_result = pthread_mutex_init(&entity->mutex_handle, 0);
    if (init_result == -1) {
        os_posix_entity_release(entity);
        entity = 0;
    }
    Mutex handle = {(u64)entity};
    return handle;
}

void os_mutex_release(Mutex mutex) {
    if (MemoryIsZeroStruct(&mutex)) {
        return;
    }
    OS_Posix_Entity *entity = (OS_Posix_Entity *)mutex.v[0];
    pthread_mutex_destroy(&entity->mutex_handle);
    os_posix_entity_release(entity);
}

void os_mutex_take(Mutex mutex) {
    if (MemoryIsZeroStruct(&mutex)) {
        return;
    }
    OS_Posix_Entity *entity = (OS_Posix_Entity *)mutex.v[0];
    pthread_mutex_lock(&entity->mutex_handle);
}

void os_mutex_drop(Mutex mutex) {
    if (MemoryIsZeroStruct(&mutex)) {
        return;
    }
    OS_Posix_Entity *entity = (OS_Posix_Entity *)mutex.v[0];
    pthread_mutex_unlock(&entity->mutex_handle);
}

RW_Mutex os_rw_mutex_alloc(void) {
    OS_Posix_Entity *entity = os_posix_entity_alloc(OS_Posix_Entity_Kind_RW_Mutex);
    int              init_result = pthread_rwlock_init(&entity->rw_mutex_handle, 0);
    if (init_result == -1) {
        os_posix_entity_release(entity);
        entity = 0;
    }
    RW_Mutex handle = {(u64)entity};
    return handle;
}

void os_rw_mutex_release(RW_Mutex rw_mutex) {
    if (MemoryIsZeroStruct(&rw_mutex)) {
        return;
    }
    OS_Posix_Entity *entity = (OS_Posix_Entity *)rw_mutex.v[0];
    pthread_rwlock_destroy(&entity->rw_mutex_handle);
    os_posix_entity_release(entity);
}

void os_rw_mutex_take(RW_Mutex rw_mutex, b32 write_mode) {
    if (MemoryIsZeroStruct(&rw_mutex)) {
        return;
    }
    OS_Posix_Entity *entity = (OS_Posix_Entity *)rw_mutex.v[0];
    if (write_mode) {
        pthread_rwlock_wrlock(&entity->rw_mutex_handle);
    } else {
        pthread_rwlock_rdlock(&entity->rw_mutex_handle);
    }
}

void os_rw_mutex_drop(RW_Mutex rw_mutex, b32 write_mode) {
    if (MemoryIsZeroStruct(&rw_mutex)) {
        return;
    }
    OS_Posix_Entity *entity = (OS_Posix_Entity *)rw_mutex.v[0];
    pthread_rwlock_unlock(&entity->rw_mutex_handle);
}

Cond_Var os_cond_var_alloc(void) {
    OS_Posix_Entity *entity = os_posix_entity_alloc(OS_Posix_Entity_Kind_Condition_Var);
    int              init_result = pthread_cond_init(&entity->cv.cond_handle, 0);
    if (init_result == -1) {
        os_posix_entity_release(entity);
        entity = 0;
    }
    int init2_result = 0;
    if (entity) {
        init2_result = pthread_mutex_init(&entity->cv.rwlock_mutex_handle, 0);
    }
    if (init2_result == -1) {
        pthread_cond_destroy(&entity->cv.cond_handle);
        os_posix_entity_release(entity);
        entity = 0;
    }
    Cond_Var handle = {(u64)entity};
    return handle;
}

void os_cond_var_release(Cond_Var cv) {
    if (MemoryIsZeroStruct(&cv)) {
        return;
    }
    OS_Posix_Entity *entity = (OS_Posix_Entity *)cv.v[0];
    pthread_cond_destroy(&entity->cv.cond_handle);
    pthread_mutex_destroy(&entity->cv.rwlock_mutex_handle);
    os_posix_entity_release(entity);
}

b32 os_cond_var_wait(Cond_Var cv, Mutex mutex, u64 endt_us) {
    if (MemoryIsZeroStruct(&cv)) {
        return 0;
    }
    if (MemoryIsZeroStruct(&mutex)) {
        return 0;
    }
    OS_Posix_Entity *cv_entity = (OS_Posix_Entity *)cv.v[0];
    OS_Posix_Entity *mutex_entity = (OS_Posix_Entity *)mutex.v[0];
    struct timespec  endt_timespec;
    endt_timespec.tv_sec = endt_us / Million(1);
    endt_timespec.tv_nsec = Thousand(1) * (endt_us - (endt_us / Million(1)) * Million(1));
    int wait_result = pthread_cond_timedwait(&cv_entity->cv.cond_handle, &mutex_entity->mutex_handle, &endt_timespec);
    b32 result = (wait_result != ETIMEDOUT);
    return result;
}

b32 os_cond_var_wait_rw(Cond_Var cv, RW_Mutex mutex_rw, b32 write_mode, u64 endt_us) {
    // POSIX limitation: pthread does not provide native condition variable + rwlock integration
    // Workaround: Use an auxiliary mutex (cv.rwlock_mutex_handle) to mediate between cv and rwlock
    if (MemoryIsZeroStruct(&cv)) {
        return 0;
    }
    if (MemoryIsZeroStruct(&mutex_rw)) {
        return 0;
    }
    OS_Posix_Entity *cv_entity = (OS_Posix_Entity *)cv.v[0];
    OS_Posix_Entity *rw_mutex_entity = (OS_Posix_Entity *)mutex_rw.v[0];
    struct timespec  endt_timespec;
    endt_timespec.tv_sec = endt_us / Million(1);
    endt_timespec.tv_nsec = Thousand(1) * (endt_us - (endt_us / Million(1)) * Million(1));
    b32 result = 0;
    pthread_mutex_lock(&cv_entity->cv.rwlock_mutex_handle);
    pthread_rwlock_unlock(&rw_mutex_entity->rw_mutex_handle);
    for (;;) {
        int wait_result = pthread_cond_timedwait(&cv_entity->cv.cond_handle, &cv_entity->cv.rwlock_mutex_handle, &endt_timespec);
        if (wait_result != ETIMEDOUT) {
            if (write_mode) {
                pthread_rwlock_wrlock(&rw_mutex_entity->rw_mutex_handle);
            } else {
                pthread_rwlock_rdlock(&rw_mutex_entity->rw_mutex_handle);
            }
            result = 1;
            break;
        }
        if (wait_result == ETIMEDOUT) {
            if (write_mode) {
                pthread_rwlock_wrlock(&rw_mutex_entity->rw_mutex_handle);
            } else {
                pthread_rwlock_rdlock(&rw_mutex_entity->rw_mutex_handle);
            }
            break;
        }
    }
    pthread_mutex_unlock(&cv_entity->cv.rwlock_mutex_handle);
    return result;
}

void os_cond_var_signal(Cond_Var cv) {
    if (MemoryIsZeroStruct(&cv)) {
        return;
    }
    OS_Posix_Entity *cv_entity = (OS_Posix_Entity *)cv.v[0];
    pthread_cond_signal(&cv_entity->cv.cond_handle);
}

void os_cond_var_broadcast(Cond_Var cv) {
    if (MemoryIsZeroStruct(&cv)) {
        return;
    }
    OS_Posix_Entity *cv_entity = (OS_Posix_Entity *)cv.v[0];
    pthread_cond_broadcast(&cv_entity->cv.cond_handle);
}

Semaphore os_semaphore_alloc(u32 initial_count, u32 max_count, String name) {
    Semaphore result = {0};
    Scratch   scratch = arena_get_scratch(0, 0);

    if (name.size > 0) {
        String name_copy = str_push_copy(scratch.arena, name);
        for (u64 attempt_idx = 0; attempt_idx < 64; attempt_idx += 1) {
            sem_t *s = sem_open((char *)name_copy.str, O_CREAT | O_EXCL, 0666, initial_count);
            if (s == SEM_FAILED) {
                s = sem_open((char *)name_copy.str, 0);
            }
            if (s != SEM_FAILED) {
                result.v[0] = (u64)s;
                break;
            }
        }
    } else {
#if OS_LINUX
        sem_t *s = mmap(0, sizeof(*s), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        AssertAlways(s != MAP_FAILED);
        int err = sem_init(s, 0, initial_count);
        if (err == 0) {
            result.v[0] = (u64)s;
        }
#elif OS_MAC
        dispatch_semaphore_t sem = dispatch_semaphore_create((long)initial_count);
        if (sem != NULL) {
            result.v[0] = (u64)CFBridgingRetain(sem);
        }
#endif
    }

    arena_end_scratch(&scratch);
    return result;
}

void os_semaphore_release(Semaphore semaphore) {
#if OS_LINUX
    int err = munmap((void *)semaphore.v[0], sizeof(sem_t));
    AssertAlways(err == 0);
#elif OS_MAC
    CFBridgingRelease((void *)semaphore.v[0]);
#endif
}

Semaphore os_semaphore_open(String name) {
    Semaphore result = {0};
    sem_t    *s = sem_open((char *)name.str, 0);
    if (s != SEM_FAILED) {
        result.v[0] = (u64)s;
    }
    return result;
}

void os_semaphore_close(Semaphore semaphore) {
    sem_t *s = (sem_t *)semaphore.v[0];
    sem_close(s);
}

b32 os_semaphore_take(Semaphore semaphore, u64 endt_us) {
#if OS_LINUX
    sem_t *s = (sem_t *)semaphore.v[0];

    if (endt_us == MAX_U64) {
        for (;;) {
            int err = sem_wait(s);
            if (err == 0) {
                return 1;
            } else if (errno == EINTR) {
                continue;
            }
            return 0;
        }
    } else {
        u64 now_us = os_now_microseconds();
        if (endt_us <= now_us) {
            return (sem_trywait(s) == 0);
        }

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        u64 timeout_us = endt_us - now_us;
        u64 timeout_sec = timeout_us / 1000000;
        u64 timeout_nsec = (timeout_us % 1000000) * 1000;

        ts.tv_sec += timeout_sec;
        ts.tv_nsec += timeout_nsec;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        for (;;) {
            int err = sem_timedwait(s, &ts);
            if (err == 0) {
                return 1;
            } else if (errno == EINTR) {
                continue;
            } else if (errno == ETIMEDOUT) {
                return 0;
            }
            return 0;
        }
    }
#elif OS_MAC
    dispatch_semaphore_t sem = (__bridge dispatch_semaphore_t)(void *)semaphore.v[0];
    long                 result;
    if (endt_us == MAX_U64) {
        result = dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    } else {
        u64 now_us = os_now_microseconds();
        if (endt_us <= now_us) {
            result = dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW);
        } else {
            u64             timeout_us = endt_us - now_us;
            dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(timeout_us * 1000));
            result = dispatch_semaphore_wait(sem, timeout);
        }
    }
    return (result == 0);
#endif
}

void os_semaphore_drop(Semaphore semaphore) {
#if OS_LINUX
    for (;;) {
        int err = sem_post((sem_t *)semaphore.v[0]);
        if (err == 0) {
            break;
        } else {
            if (errno == EAGAIN) {
                continue;
            }
        }
        break;
    }
#elif OS_MAC
    dispatch_semaphore_t sem = (__bridge dispatch_semaphore_t)(void *)semaphore.v[0];
    dispatch_semaphore_signal(sem);
#endif
}


#if OS_LINUX
Barrier os_barrier_alloc(u64 count) {
    OS_Posix_Entity *entity = os_posix_entity_alloc(OS_Posix_Entity_Kind_Barrier);
    if (entity != 0) {
        pthread_barrier_init(&entity->barrier, 0, count);
    }
    Barrier result = {(u64)entity};
    return result;
}

void os_barrier_release(Barrier barrier) {
    OS_Posix_Entity *entity = (OS_Posix_Entity *)barrier.v[0];
    if (entity != 0) {
        pthread_barrier_destroy(&entity->barrier);
        os_posix_entity_release(entity);
    }
}

void os_barrier_wait(Barrier barrier) {
    OS_Posix_Entity *entity = (OS_Posix_Entity *)barrier.v[0];
    if (entity != 0) {
        pthread_barrier_wait(&entity->barrier);
    }
}
#elif OS_MAC
// macOS doesn't have pthread_barrier_t, so we implement it using mutex and cond var
Barrier os_barrier_alloc(u64 count) {
    OS_Posix_Entity *entity = os_posix_entity_alloc(OS_Posix_Entity_Kind_Barrier);
    if (entity != 0) {
        entity->barrier.count = 0;
        entity->barrier.trip_count = count;
        pthread_mutex_init(&entity->barrier.mutex, 0);
        pthread_cond_init(&entity->barrier.cond, 0);
    }
    Barrier result = {(u64)entity};
    return result;
}

void os_barrier_release(Barrier barrier) {
    OS_Posix_Entity *entity = (OS_Posix_Entity *)barrier.v[0];
    if (entity != 0) {
        pthread_mutex_destroy(&entity->barrier.mutex);
        pthread_cond_destroy(&entity->barrier.cond);
        os_posix_entity_release(entity);
    }
}

void os_barrier_wait(Barrier barrier) {
    OS_Posix_Entity *entity = (OS_Posix_Entity *)barrier.v[0];
    if (entity != 0) {
        pthread_mutex_lock(&entity->barrier.mutex);
        entity->barrier.count += 1;
        if (entity->barrier.count >= entity->barrier.trip_count) {
            entity->barrier.count = 0;
            pthread_cond_broadcast(&entity->barrier.cond);
        } else {
            pthread_cond_wait(&entity->barrier.cond, &entity->barrier.mutex);
        }
        pthread_mutex_unlock(&entity->barrier.mutex);
    }
}
#endif

OS_Handle os_library_open(String path) {
    Scratch   scratch = arena_get_scratch(0, 0);
    char     *path_cstr = (char *)str_push_copy(scratch.arena, path).str;
    void     *so = dlopen(path_cstr, RTLD_LAZY | RTLD_LOCAL);
    OS_Handle lib = {(u64)so};
    arena_end_scratch(&scratch);
    return lib;
}

void *os_library_load_proc(OS_Handle lib, String name) {
    Scratch scratch = arena_get_scratch(0, 0);
    void   *so = (void *)lib.v[0];
    char   *name_cstr = (char *)str_push_copy(scratch.arena, name).str;
    void   *proc = dlsym(so, name_cstr);
    arena_end_scratch(&scratch);
    return proc;
}

void os_library_close(OS_Handle lib) {
    void *so = (void *)lib.v[0];
    dlclose(so);
}

void os_safe_call(Thread_Entry_Point *func, Thread_Entry_Point *fail_handler, void *ptr) {
    OS_Posix_Safe_Call_Chain chain = {0};
    SLLStackPush(os_posix_safe_call_chain, &chain);
    chain.fail_handler = fail_handler;
    chain.ptr = ptr;

    struct sigaction new_act = {0};
    new_act.sa_handler = os_posix_safe_call_sig_handler;
    int signals_to_handle[] = {
        SIGILL,
        SIGFPE,
        SIGSEGV,
        SIGBUS,
        SIGTRAP,
    };
    struct sigaction og_act[ArrayCount(signals_to_handle)] = {0};

    for (u32 i = 0; i < ArrayCount(signals_to_handle); i += 1) {
        sigaction(signals_to_handle[i], &new_act, &og_act[i]);
    }

    func(ptr);

    for (u32 i = 0; i < ArrayCount(signals_to_handle); i += 1) {
        sigaction(signals_to_handle[i], &og_act[i], 0);
    }
}

typedef struct Guid Guid;
struct Guid {
    union {
        u8 v[16];
        struct {
            u32 data1;
            u16 data2;
            u16 data3;
            u8  data4[8];
        };
    };
};

Guid os_make_guid(void) {
    Guid guid = {0};
#if OS_LINUX
    getrandom(guid.v, sizeof(guid.v), 0);
#elif OS_MAC
    arc4random_buf(guid.v, sizeof(guid.v));
#endif
    guid.data3 &= 0x0fff;
    guid.data3 |= (4 << 12);
    guid.data4[0] &= 0x3f;
    guid.data4[0] |= 0x80;
    return guid;
}
