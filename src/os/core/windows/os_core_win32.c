#include "os_core_win32.h"

static inline OS_Handle
os_handle_zero(void) {
    OS_Handle result = {0};
    return result;
}

static inline b32
os_handle_match(OS_Handle a, OS_Handle b) {
    return a.v[0] == b.v[0];
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

static Date_Time os_w32_date_time_from_system_time(SYSTEMTIME *in) {
    Date_Time dt = {0};
    dt.year = in->wYear;
    dt.mon = in->wMonth - 1;
    dt.day = in->wDay - 1;
    dt.hour = in->wHour;
    dt.min = in->wMinute;
    dt.sec = in->wSecond;
    dt.msec = in->wMilliseconds;
    return dt;
}

static SYSTEMTIME os_w32_system_time_from_date_time(Date_Time dt) {
    SYSTEMTIME result = {0};
    result.wYear = (WORD)dt.year;
    result.wMonth = dt.mon + 1;
    result.wDay = dt.day + 1;
    result.wHour = dt.hour;
    result.wMinute = dt.min;
    result.wSecond = dt.sec;
    result.wMilliseconds = dt.msec;
    return result;
}

static Dense_Time os_w32_dense_time_from_file_time(FILETIME *in) {
    SYSTEMTIME systime = {0};
    FileTimeToSystemTime(in, &systime);
    Date_Time  date_time = os_w32_date_time_from_system_time(&systime);
    Dense_Time result = dense_time_from_date_time(date_time);
    return result;
}

static FILETIME os_w32_file_time_from_date_time(Date_Time dt) {
    SYSTEMTIME systime = os_w32_system_time_from_date_time(dt);
    FILETIME   result = {0};
    SystemTimeToFileTime(&systime, &result);
    return result;
}

static File_Properties os_w32_file_properties_from_find_data(WIN32_FIND_DATAW *find_data) {
    File_Properties props = {0};
    props.size = ((u64)find_data->nFileSizeHigh << 32) | (u64)find_data->nFileSizeLow;
    props.create_time = os_w32_dense_time_from_file_time(&find_data->ftCreationTime);
    props.modify_time = os_w32_dense_time_from_file_time(&find_data->ftLastWriteTime);
    if (find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        props.flags |= File_Property_Flag_Is_Folder;
    }
    return props;
}

static u32 os_w32_sleep_ms_from_endt_us(u64 endt_us) {
    u32 sleep_ms = 0;
    if (endt_us == max_U64) {
        sleep_ms = INFINITE;
    } else {
        u64 now_us = os_now_microseconds();
        if (now_us < endt_us) {
            u64 sleep_us = endt_us - now_us;
            sleep_ms = (u32)((sleep_us + 999) / 1000);
        }
    }
    return sleep_ms;
}

static OS_W32_Entity *os_w32_entity_alloc(OS_W32_Entity_Kind kind) {
    OS_W32_Entity *entity = 0;
    EnterCriticalSection(&os_w32_state.entity_mutex);
    {
        entity = os_w32_state.entity_free;
        if (entity) {
            SLLStackPop(os_w32_state.entity_free);
        } else {
            entity = push_array_no_zero(os_w32_state.entity_arena, OS_W32_Entity, 1);
        }
    }
    LeaveCriticalSection(&os_w32_state.entity_mutex);
    MemoryZeroStruct(entity);
    entity->kind = kind;
    return entity;
}

static void os_w32_entity_release(OS_W32_Entity *entity) {
    EnterCriticalSection(&os_w32_state.entity_mutex);
    {
        SLLStackPush(os_w32_state.entity_free, entity);
    }
    LeaveCriticalSection(&os_w32_state.entity_mutex);
}

static DWORD WINAPI os_w32_thread_entry_point(void *ptr) {
    TCTX *tctx = tctx_alloc();
    tctx_select(tctx);

    OS_W32_Entity      *entity = (OS_W32_Entity *)ptr;
    Thread_Entry_Point *func = entity->thread.func;
    void               *thread_ptr = entity->thread.ptr;

    func(thread_ptr);

    tctx_release(tctx);

    return 0;
}

OS_System_Info *os_get_system_info(void) {
    return &os_w32_state.system_info;
}

OS_Process_Info *os_get_process_info(void) {
    return &os_w32_state.process_info;
}

String os_get_current_path(Arena *arena) {
    Scratch scratch = arena_get_scratch(&arena, 1);
    DWORD   length = GetCurrentDirectoryW(0, 0);
    u16    *memory = push_array_no_zero(scratch.arena, u16, length + 1);
    length = GetCurrentDirectoryW(length + 1, (WCHAR *)memory);
    String16 path16 = {memory, length};
    String   result = str_from_str16(arena, path16);
    arena_end_scratch(&scratch);
    return result;
}

u32 os_get_process_start_time_unix(void) {
    HANDLE   handle = GetCurrentProcess();
    FILETIME create_time = {0};
    FILETIME exit_time, kernel_time, user_time;
    if (GetProcessTimes(handle, &create_time, &exit_time, &kernel_time, &user_time)) {
        u64 win32_time = ((u64)create_time.dwHighDateTime << 32) | create_time.dwLowDateTime;
        u64 unix_time = (win32_time - 116444736000000000ULL) / 10000000;
        return (u32)unix_time;
    }
    return 0;
}

void *os_reserve(u64 size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return result;
}

b32 os_commit(void *ptr, u64 size) {
    b32 result = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return result;
}

void os_decommit(void *ptr, u64 size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

void os_mem_release(void *ptr, u64 size) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

void *os_reserve_large(u64 size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
    return result;
}

b32 os_commit_large(void *ptr, u64 size) {
    return 1;
}

u32 os_tid(void) {
    DWORD id = GetCurrentThreadId();
    return (u32)id;
}

void os_set_thread_name(String name) {
    Scratch  scratch = arena_get_scratch(0, 0);
    String16 name16 = str16_from_str(scratch.arena, name);

    typedef HRESULT                   SetThreadDescription_Type(HANDLE hThread, PCWSTR lpThreadDescription);
    static SetThreadDescription_Type *SetThreadDescription_func = 0;
    static b32                        tried_load = 0;

    if (!tried_load) {
        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
        if (kernel32) {
            SetThreadDescription_func = (SetThreadDescription_Type *)GetProcAddress(kernel32, "SetThreadDescription");
        }
        tried_load = 1;
    }

    if (SetThreadDescription_func) {
        SetThreadDescription_func(GetCurrentThread(), (WCHAR *)name16.str);
    }

    arena_end_scratch(&scratch);
}

void os_abort(s32 exit_code) {
    ExitProcess(exit_code);
}

OS_Handle os_file_open(OS_Access_Flags flags, String path) {
    OS_Handle result = {0};
    Scratch   scratch = arena_get_scratch(0, 0);
    String16  path16 = str16_from_str(scratch.arena, path);

    DWORD               access_flags = 0;
    DWORD               share_mode = 0;
    DWORD               creation_disposition = OPEN_EXISTING;
    SECURITY_ATTRIBUTES security_attributes = {sizeof(security_attributes), 0, 0};

    if (flags & OS_Access_Flag_Read) {
        access_flags |= GENERIC_READ;
    }
    if (flags & OS_Access_Flag_Write) {
        access_flags |= GENERIC_WRITE;
        creation_disposition = CREATE_ALWAYS;
    }
    if (flags & OS_Access_Flag_Execute) {
        access_flags |= GENERIC_EXECUTE;
    }
    if (flags & OS_Access_Flag_Share_Read) {
        share_mode |= FILE_SHARE_READ;
    }
    if (flags & OS_Access_Flag_Share_Write) {
        share_mode |= FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    }
    if (flags & OS_Access_Flag_Append) {
        creation_disposition = OPEN_ALWAYS;
        access_flags |= FILE_APPEND_DATA;
    }
    if (flags & OS_Access_Flag_Inherited) {
        security_attributes.bInheritHandle = 1;
    }

    HANDLE file = CreateFileW((WCHAR *)path16.str, access_flags, share_mode,
                              &security_attributes, creation_disposition,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (file != INVALID_HANDLE_VALUE) {
        result.v[0] = (u64)file;
    }

    arena_end_scratch(&scratch);
    return result;
}

void os_file_close(OS_Handle file) {
    if (os_handle_match(file, os_handle_zero())) {
        return;
    }
    HANDLE handle = (HANDLE)file.v[0];
    CloseHandle(handle);
}

u64 os_file_read(OS_Handle file, Rng1_u64 rng, void *out_data) {
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    HANDLE handle = (HANDLE)file.v[0];

    u64 total_read_size = 0;
    u64 to_read = rng.max - rng.min;
    u64 offset = rng.min;

    for (; total_read_size < to_read;) {
        u64        amt64 = to_read - total_read_size;
        u32        amt32 = (amt64 > max_U32) ? max_U32 : (u32)amt64;
        DWORD      read_size = 0;
        OVERLAPPED overlapped = {0};
        overlapped.Offset = (offset & 0x00000000ffffffffull);
        overlapped.OffsetHigh = (offset & 0xffffffff00000000ull) >> 32;

        if (!ReadFile(handle, (u8 *)out_data + total_read_size, amt32, &read_size, &overlapped)) {
            break;
        }

        offset += read_size;
        total_read_size += read_size;

        if (read_size != amt32) {
            break;
        }
    }

    return total_read_size;
}

u64 os_file_write(OS_Handle file, Rng1_u64 rng, void *data) {
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    HANDLE handle = (HANDLE)file.v[0];

    u64 total_write_size = rng.max - rng.min;
    u64 total_written = 0;
    u64 offset = rng.min;

    for (; total_written < total_write_size;) {
        u64        bytes_left = total_write_size - total_written;
        u32        write_size = (bytes_left > MB(1)) ? MB(1) : (u32)bytes_left;
        DWORD      bytes_written = 0;
        OVERLAPPED overlapped = {0};
        overlapped.Offset = (offset & 0x00000000ffffffffull);
        overlapped.OffsetHigh = (offset & 0xffffffff00000000ull) >> 32;

        if (!WriteFile(handle, (u8 *)data + total_written, write_size, &bytes_written, &overlapped)) {
            break;
        }

        total_written += bytes_written;
        offset += bytes_written;

        if (bytes_left == 0) {
            break;
        }
    }

    return total_written;
}

b32 os_file_set_times(OS_Handle file, Date_Time date_time) {
    if (os_handle_match(file, os_handle_zero())) {
        return 0;
    }
    HANDLE   handle = (HANDLE)file.v[0];
    FILETIME file_time = os_w32_file_time_from_date_time(date_time);
    b32      result = SetFileTime(handle, &file_time, &file_time, &file_time);
    return result;
}

File_Properties os_properties_from_file(OS_Handle file) {
    File_Properties props = {0};
    if (os_handle_match(file, os_handle_zero())) {
        return props;
    }

    HANDLE                     handle = (HANDLE)file.v[0];
    BY_HANDLE_FILE_INFORMATION info;

    if (GetFileInformationByHandle(handle, &info)) {
        props.size = ((u64)info.nFileSizeHigh << 32) | (u64)info.nFileSizeLow;
        props.create_time = os_w32_dense_time_from_file_time(&info.ftCreationTime);
        props.modify_time = os_w32_dense_time_from_file_time(&info.ftLastWriteTime);

        if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            props.flags |= File_Property_Flag_Is_Folder;
        }
    }

    return props;
}

OS_File_ID os_id_from_file(OS_Handle file) {
    OS_File_ID result = {0};
    if (os_handle_match(file, os_handle_zero())) {
        return result;
    }

    HANDLE                     handle = (HANDLE)file.v[0];
    BY_HANDLE_FILE_INFORMATION info;

    if (GetFileInformationByHandle(handle, &info)) {
        result.v[0] = info.dwVolumeSerialNumber;
        result.v[1] = info.nFileIndexLow;
        result.v[2] = info.nFileIndexHigh;
    }

    return result;
}

b32 os_delete_file_at_path(String path) {
    Scratch  scratch = arena_get_scratch(0, 0);
    String16 path16 = str16_from_str(scratch.arena, path);
    b32      result = DeleteFileW((WCHAR *)path16.str);
    arena_end_scratch(&scratch);
    return result;
}

b32 os_copy_file_path(String dst, String src) {
    Scratch  scratch = arena_get_scratch(0, 0);
    String16 src16 = str16_from_str(scratch.arena, src);
    String16 dst16 = str16_from_str(scratch.arena, dst);
    b32      result = CopyFileW((WCHAR *)src16.str, (WCHAR *)dst16.str, 0);
    arena_end_scratch(&scratch);
    return result;
}

b32 os_move_file_path(String dst, String src) {
    Scratch  scratch = arena_get_scratch(0, 0);
    String16 src16 = str16_from_str(scratch.arena, src);
    String16 dst16 = str16_from_str(scratch.arena, dst);
    b32      result = MoveFileW((WCHAR *)src16.str, (WCHAR *)dst16.str);
    arena_end_scratch(&scratch);
    return result;
}

String os_full_path_from_path(Arena *arena, String path) {
    Scratch  scratch = arena_get_scratch(&arena, 1);
    String16 path16 = str16_from_str(scratch.arena, path);

    DWORD length = GetFullPathNameW((WCHAR *)path16.str, 0, 0, 0);
    u16  *buffer = push_array_no_zero(scratch.arena, u16, length + 1);
    GetFullPathNameW((WCHAR *)path16.str, length + 1, (WCHAR *)buffer, 0);

    String16 full_path16 = {buffer, length};
    String   result = str_from_str16(arena, full_path16);
    arena_end_scratch(&scratch);
    return result;
}

b32 os_file_path_exists(String path) {
    Scratch  scratch = arena_get_scratch(0, 0);
    String16 path16 = str16_from_str(scratch.arena, path);
    DWORD    attrib = GetFileAttributesW((WCHAR *)path16.str);
    b32      result = (attrib != INVALID_FILE_ATTRIBUTES);
    arena_end_scratch(&scratch);
    return result;
}

b32 os_folder_path_exists(String path) {
    Scratch  scratch = arena_get_scratch(0, 0);
    String16 path16 = str16_from_str(scratch.arena, path);
    DWORD    attrib = GetFileAttributesW((WCHAR *)path16.str);
    b32      result = (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
    arena_end_scratch(&scratch);
    return result;
}

File_Properties os_properties_from_file_path(String path) {
    File_Properties props = {0};
    Scratch         scratch = arena_get_scratch(0, 0);
    String16        path16 = str16_from_str(scratch.arena, path);

    WIN32_FILE_ATTRIBUTE_DATA attrib_data;
    if (GetFileAttributesExW((WCHAR *)path16.str, GetFileExInfoStandard, &attrib_data)) {
        props.size = ((u64)attrib_data.nFileSizeHigh << 32) | (u64)attrib_data.nFileSizeLow;
        props.create_time = os_w32_dense_time_from_file_time(&attrib_data.ftCreationTime);
        props.modify_time = os_w32_dense_time_from_file_time(&attrib_data.ftLastWriteTime);

        if (attrib_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            props.flags |= File_Property_Flag_Is_Folder;
        }
    }

    arena_end_scratch(&scratch);
    return props;
}

OS_Handle os_file_map_open(OS_Access_Flags flags, OS_Handle file) {
    OS_Handle result = {0};
    if (os_handle_match(file, os_handle_zero())) {
        return result;
    }

    HANDLE file_handle = (HANDLE)file.v[0];
    DWORD  protect = 0;

    if (flags & OS_Access_Flag_Write) {
        protect = PAGE_READWRITE;
    } else if (flags & OS_Access_Flag_Read) {
        protect = PAGE_READONLY;
    }

    HANDLE map = CreateFileMappingW(file_handle, 0, protect, 0, 0, 0);
    if (map != 0) {
        result.v[0] = (u64)map;
    }

    return result;
}

void os_file_map_close(OS_Handle map) {
    if (os_handle_match(map, os_handle_zero())) {
        return;
    }
    HANDLE handle = (HANDLE)map.v[0];
    CloseHandle(handle);
}

void *os_file_map_view_open(OS_Handle map, OS_Access_Flags flags, Rng1_u64 range) {
    if (os_handle_match(map, os_handle_zero())) {
        return 0;
    }

    HANDLE map_handle = (HANDLE)map.v[0];
    DWORD  access = 0;

    if (flags & OS_Access_Flag_Write) {
        access = FILE_MAP_WRITE;
    } else if (flags & OS_Access_Flag_Read) {
        access = FILE_MAP_READ;
    }

    u64 offset = range.min;
    u64 size = range.max - range.min;

    void *base = MapViewOfFile(map_handle, access,
                               (offset >> 32) & 0xFFFFFFFF,
                               offset & 0xFFFFFFFF,
                               size);
    return base;
}

void os_file_map_view_close(OS_Handle map, void *ptr, Rng1_u64 range) {
    UnmapViewOfFile(ptr);
}

OS_File_Iter *os_file_iter_begin(Arena *arena, String path, OS_Filter_Iter_Flags flags) {
    OS_File_Iter *base_iter = push_array(arena, OS_File_Iter, 1);
    base_iter->flags = flags;

    OS_W32_File_Iter *iter = (OS_W32_File_Iter *)base_iter->memory;

    Scratch  scratch = arena_get_scratch(&arena, 1);
    String   search_path = str_pushf(scratch.arena, "%S/*", path);
    String16 search_path16 = str16_from_str(scratch.arena, search_path);

    iter->handle = FindFirstFileW((WCHAR *)search_path16.str, &iter->find_data);
    iter->first = 1;

    arena_end_scratch(&scratch);
    return base_iter;
}

b32 os_file_iter_next(Arena *arena, OS_File_Iter *iter, File_Properties *info_out) {
    b32               good = 0;
    OS_W32_File_Iter *win32_iter = (OS_W32_File_Iter *)iter->memory;

    if (win32_iter->handle == INVALID_HANDLE_VALUE) {
        return 0;
    }

    for (;;) {
        if (win32_iter->first) {
            win32_iter->first = 0;
            good = 1;
        } else {
            good = FindNextFileW(win32_iter->handle, &win32_iter->find_data);
        }

        if (!good) {
            break;
        }

        WCHAR *name = win32_iter->find_data.cFileName;
        b32    is_dots = (name[0] == L'.' && (name[1] == 0 || (name[1] == L'.' && name[2] == 0)));
        b32    is_folder = (win32_iter->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

        b32 filtered = (is_dots ||
                        (is_folder && (iter->flags & OS_Filter_Iter_Flag_Skip_Folders)) ||
                        (!is_folder && (iter->flags & OS_Filter_Iter_Flag_Skip_Files)));

        if (!filtered) {
            u64 name_len = 0;
            for (; name[name_len]; name_len++)
                String16 name16 = {(u16 *)name, name_len};

            *info_out = os_w32_file_properties_from_find_data(&win32_iter->find_data);
            info_out->name = str_from_str16(arena, name16);
            break;
        }
    }

    return good;
}

void os_file_iter_end(OS_File_Iter *iter) {
    OS_W32_File_Iter *win32_iter = (OS_W32_File_Iter *)iter->memory;
    if (win32_iter->handle != INVALID_HANDLE_VALUE) {
        FindClose(win32_iter->handle);
    }
}

b32 os_make_directory(String path) {
    Scratch  scratch = arena_get_scratch(0, 0);
    String16 path16 = str16_from_str(scratch.arena, path);
    b32      result = CreateDirectoryW((WCHAR *)path16.str, 0);
    arena_end_scratch(&scratch);
    return result;
}

OS_Handle os_shared_memory_alloc(u64 size, String name) {
    Scratch  scratch = arena_get_scratch(0, 0);
    String16 name16 = {0};
    if (name.size > 0) {
        name16 = str16_from_str(scratch.arena, name);
    }

    HANDLE    handle = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE,
                                          (size >> 32) & 0xFFFFFFFF, size & 0xFFFFFFFF,
                                       name16.size > 0 ? (WCHAR *)name16.str : 0);
    OS_Handle result = {(u64)handle};
    arena_end_scratch(&scratch);
    return result;
}

OS_Handle os_shared_memory_open(String name) {
    Scratch   scratch = arena_get_scratch(0, 0);
    String16  name16 = str16_from_str(scratch.arena, name);
    HANDLE    handle = OpenFileMappingW(FILE_MAP_ALL_ACCESS, 0, (WCHAR *)name16.str);
    OS_Handle result = {(u64)handle};
    arena_end_scratch(&scratch);
    return result;
}

void os_shared_memory_close(OS_Handle handle) {
    if (os_handle_match(handle, os_handle_zero())) {
        return;
    }
    CloseHandle((HANDLE)handle.v[0]);
}

void *os_shared_memory_view_open(OS_Handle handle, Rng1_u64 range) {
    if (os_handle_match(handle, os_handle_zero())) {
        return 0;
    }

    HANDLE map_handle = (HANDLE)handle.v[0];
    u64    offset = range.min;
    u64    size = range.max - range.min;

    void *base = MapViewOfFile(map_handle, FILE_MAP_ALL_ACCESS,
                               (offset >> 32) & 0xFFFFFFFF,
                               offset & 0xFFFFFFFF,
                               size);
    return base;
}

void os_shared_memory_view_close(OS_Handle handle, void *ptr, Rng1_u64 range) {
    UnmapViewOfFile(ptr);
}

u64 os_now_microseconds(void) {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    u64 microseconds = (counter.QuadPart * Million(1)) / os_w32_state.counts_per_second;
    return microseconds;
}

u32 os_now_unix(void) {
    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);
    u64 win32_time = ((u64)file_time.dwHighDateTime << 32) | file_time.dwLowDateTime;
    u64 unix_time = (win32_time - 116444736000000000ULL) / 10000000;
    return (u32)unix_time;
}

Date_Time os_now_universal_time(void) {
    SYSTEMTIME systime;
    GetSystemTime(&systime);
    Date_Time result = os_w32_date_time_from_system_time(&systime);
    return result;
}

Date_Time os_universal_time_from_local(Date_Time *date_time) {
    SYSTEMTIME local_time = os_w32_system_time_from_date_time(*date_time);
    SYSTEMTIME universal_time;
    TzSpecificLocalTimeToSystemTime(0, &local_time, &universal_time);
    Date_Time result = os_w32_date_time_from_system_time(&universal_time);
    return result;
}

Date_Time os_local_time_from_universal(Date_Time *date_time) {
    SYSTEMTIME universal_time = os_w32_system_time_from_date_time(*date_time);
    SYSTEMTIME local_time;
    SystemTimeToTzSpecificLocalTime(0, &universal_time, &local_time);
    Date_Time result = os_w32_date_time_from_system_time(&local_time);
    return result;
}

void os_sleep_milliseconds(u32 msec) {
    Sleep(msec);
}

OS_Handle os_process_launch(OS_Process_Launch_Params *params) {
    OS_Handle result = {0};
    Scratch   scratch = arena_get_scratch(0, 0);

    String   cmd_line_str = str_list_join(scratch.arena, &params->cmd_line, 0);
    String16 cmd_line16 = str16_from_str(scratch.arena, cmd_line_str);

    u16 *env_block = 0;
    if (!params->inherit_env) {
        u64 total_size = 0;
        for (String_Node *n = params->env.first; n != 0; n = n->next) {
            String16 str16 = str16_from_str(scratch.arena, n->string);
            total_size += str16.size + 1;
        }
        total_size += 1;

        env_block = push_array(scratch.arena, u16, total_size);
        u64 offset = 0;
        for (String_Node *n = params->env.first; n != 0; n = n->next) {
            String16 str16 = str16_from_str(scratch.arena, n->string);
            MemoryCopy(env_block + offset, str16.str, str16.size * sizeof(u16));
            offset += str16.size;
            env_block[offset++] = 0;
        }
        env_block[offset] = 0;
    }

    STARTUPINFOW startup_info = {0};
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdOutput = (HANDLE)params->stdout_file.v[0];
    startup_info.hStdError = (HANDLE)params->stderr_file.v[0];
    startup_info.hStdInput = (HANDLE)params->stdin_file.v[0];

    if (params->consoleless) {
        startup_info.dwFlags |= STARTF_USESHOWWINDOW;
        startup_info.wShowWindow = SW_HIDE;
    }

    DWORD creation_flags = 0;
    if (params->debug_process) {
        creation_flags |= DEBUG_PROCESS;
    }
    if (env_block) {
        creation_flags |= CREATE_UNICODE_ENVIRONMENT;
    }

    PROCESS_INFORMATION process_info = {0};
    String16            path16 = str16_from_str(scratch.arena, params->path);

    if (CreateProcessW(0, (WCHAR *)cmd_line16.str, 0, 0, 1, creation_flags,
                       env_block, (WCHAR *)path16.str, &startup_info, &process_info)) {
        CloseHandle(process_info.hThread);
        result.v[0] = (u64)process_info.hProcess;
    }

    arena_end_scratch(&scratch);
    return result;
}

b32 os_process_join(OS_Handle handle, u64 endt_us, u64 *exit_code_out) {
    if (os_handle_match(handle, os_handle_zero())) {
        return 0;
    }

    HANDLE process = (HANDLE)handle.v[0];
    u32    wait_ms = os_w32_sleep_ms_from_endt_us(endt_us);
    DWORD  wait_result = WaitForSingleObject(process, wait_ms);
    b32    result = (wait_result == WAIT_OBJECT_0);

    if (result && exit_code_out) {
        DWORD exit_code = 0;
        GetExitCodeProcess(process, &exit_code);
        *exit_code_out = exit_code;
    }

    return result;
}

void os_process_detach(OS_Handle handle) {
    if (os_handle_match(handle, os_handle_zero())) {
        return;
    }
    CloseHandle((HANDLE)handle.v[0]);
}

b32 os_process_kill(OS_Handle handle) {
    if (os_handle_match(handle, os_handle_zero())) {
        return 0;
    }
    HANDLE process = (HANDLE)handle.v[0];
    b32    result = TerminateProcess(process, 1);
    return result;
}

Thread os_thread_launch(Thread_Entry_Point *func, void *ptr) {
    OS_W32_Entity *entity = os_w32_entity_alloc(OS_W32_Entity_Kind_Thread);
    entity->thread.func = func;
    entity->thread.ptr = ptr;

    DWORD tid;
    entity->thread.handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)os_w32_thread_entry_point,
                                         entity, 0, &tid);
    entity->thread.tid = tid;

    Thread result = {(u64)entity};
    return result;
}

b32 os_thread_join(Thread handle, u64 endt_us) {
    if (MemoryIsZeroStruct(&handle)) {
        return 0;
    }

    OS_W32_Entity *entity = (OS_W32_Entity *)handle.v[0];
    u32            wait_ms = os_w32_sleep_ms_from_endt_us(endt_us);
    DWORD          wait_result = WaitForSingleObject(entity->thread.handle, wait_ms);
    b32            result = (wait_result == WAIT_OBJECT_0);

    if (result) {
        CloseHandle(entity->thread.handle);
        os_w32_entity_release(entity);
    }

    return result;
}

void os_thread_detach(Thread handle) {
    if (MemoryIsZeroStruct(&handle)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)handle.v[0];
    CloseHandle(entity->thread.handle);
    os_w32_entity_release(entity);
}

Mutex os_mutex_alloc(void) {
    OS_W32_Entity *entity = os_w32_entity_alloc(OS_W32_Entity_Kind_Mutex);
    InitializeCriticalSection(&entity->mutex_handle);
    Mutex result = {(u64)entity};
    return result;
}

void os_mutex_release(Mutex mutex) {
    if (MemoryIsZeroStruct(&mutex)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)mutex.v[0];
    DeleteCriticalSection(&entity->mutex_handle);
    os_w32_entity_release(entity);
}

void os_mutex_take(Mutex mutex) {
    if (MemoryIsZeroStruct(&mutex)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)mutex.v[0];
    EnterCriticalSection(&entity->mutex_handle);
}

void os_mutex_drop(Mutex mutex) {
    if (MemoryIsZeroStruct(&mutex)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)mutex.v[0];
    LeaveCriticalSection(&entity->mutex_handle);
}

RW_Mutex os_rw_mutex_alloc(void) {
    OS_W32_Entity *entity = os_w32_entity_alloc(OS_W32_Entity_Kind_RW_Mutex);
    InitializeSRWLock(&entity->rw_mutex_handle);
    RW_Mutex result = {(u64)entity};
    return result;
}

void os_rw_mutex_release(RW_Mutex rw_mutex) {
    if (MemoryIsZeroStruct(&rw_mutex)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)rw_mutex.v[0];
    os_w32_entity_release(entity);
}

void os_rw_mutex_take(RW_Mutex rw_mutex, b32 write_mode) {
    if (MemoryIsZeroStruct(&rw_mutex)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)rw_mutex.v[0];
    if (write_mode) {
        AcquireSRWLockExclusive(&entity->rw_mutex_handle);
    } else {
        AcquireSRWLockShared(&entity->rw_mutex_handle);
    }
}

void os_rw_mutex_drop(RW_Mutex rw_mutex, b32 write_mode) {
    if (MemoryIsZeroStruct(&rw_mutex)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)rw_mutex.v[0];
    if (write_mode) {
        ReleaseSRWLockExclusive(&entity->rw_mutex_handle);
    } else {
        ReleaseSRWLockShared(&entity->rw_mutex_handle);
    }
}

Cond_Var os_cond_var_alloc(void) {
    OS_W32_Entity *entity = os_w32_entity_alloc(OS_W32_Entity_Kind_ConditionVariable);
    InitializeConditionVariable(&entity->cv.cond_handle);
    InitializeCriticalSection(&entity->cv.mutex_handle);
    Cond_Var result = {(u64)entity};
    return result;
}

void os_cond_var_release(Cond_Var cv) {
    if (MemoryIsZeroStruct(&cv)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)cv.v[0];
    DeleteCriticalSection(&entity->cv.mutex_handle);
    os_w32_entity_release(entity);
}

b32 os_cond_var_wait(Cond_Var cv, Mutex mutex, u64 endt_us) {
    if (MemoryIsZeroStruct(&cv) || MemoryIsZeroStruct(&mutex)) {
        return 0;
    }

    OS_W32_Entity *cv_entity = (OS_W32_Entity *)cv.v[0];
    OS_W32_Entity *mutex_entity = (OS_W32_Entity *)mutex.v[0];

    u32  wait_ms = os_w32_sleep_ms_from_endt_us(endt_us);
    BOOL result = SleepConditionVariableCS(&cv_entity->cv.cond_handle,
                                           &mutex_entity->mutex_handle,
                                           wait_ms);
    return result != 0;
}

b32 os_cond_var_wait_rw(Cond_Var cv, RW_Mutex mutex_rw, b32 write_mode, u64 endt_us) {
    if (MemoryIsZeroStruct(&cv) || MemoryIsZeroStruct(&mutex_rw)) {
        return 0;
    }

    OS_W32_Entity *cv_entity = (OS_W32_Entity *)cv.v[0];
    OS_W32_Entity *rw_mutex_entity = (OS_W32_Entity *)mutex_rw.v[0];

    u32   wait_ms = os_w32_sleep_ms_from_endt_us(endt_us);
    ULONG flags = write_mode ? 0 : CONDITION_VARIABLE_LOCKMODE_SHARED;
    BOOL  result = SleepConditionVariableSRW(&cv_entity->cv.cond_handle,
                                             &rw_mutex_entity->rw_mutex_handle,
                                             wait_ms, flags);
    return result != 0;
}

void os_cond_var_signal(Cond_Var cv) {
    if (MemoryIsZeroStruct(&cv)) {
        return;
    }
    OS_W32_Entity *cv_entity = (OS_W32_Entity *)cv.v[0];
    WakeConditionVariable(&cv_entity->cv.cond_handle);
}

void os_cond_var_broadcast(Cond_Var cv) {
    if (MemoryIsZeroStruct(&cv)) {
        return;
    }
    OS_W32_Entity *cv_entity = (OS_W32_Entity *)cv.v[0];
    WakeAllConditionVariable(&cv_entity->cv.cond_handle);
}

Semaphore os_semaphore_alloc(u32 initial_count, u32 max_count, String name) {
    Scratch  scratch = arena_get_scratch(0, 0);
    String16 name16 = {0};
    if (name.size > 0) {
        name16 = str16_from_str(scratch.arena, name);
    }

    HANDLE    sem = CreateSemaphoreW(0, initial_count, max_count,
                                  name16.size > 0 ? (WCHAR *)name16.str : 0);
    Semaphore result = {(u64)sem};
    arena_end_scratch(&scratch);
    return result;
}

void os_semaphore_release(Semaphore semaphore) {
    if (MemoryIsZeroStruct(&semaphore)) {
        return;
    }
    CloseHandle((HANDLE)semaphore.v[0]);
}

Semaphore os_semaphore_open(String name) {
    Scratch   scratch = arena_get_scratch(0, 0);
    String16  name16 = str16_from_str(scratch.arena, name);
    HANDLE    sem = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, 0, (WCHAR *)name16.str);
    Semaphore result = {(u64)sem};
    arena_end_scratch(&scratch);
    return result;
}

void os_semaphore_close(Semaphore semaphore) {
    if (MemoryIsZeroStruct(&semaphore)) {
        return;
    }
    CloseHandle((HANDLE)semaphore.v[0]);
}

b32 os_semaphore_take(Semaphore semaphore, u64 endt_us) {
    if (MemoryIsZeroStruct(&semaphore)) {
        return 0;
    }
    u32   wait_ms = os_w32_sleep_ms_from_endt_us(endt_us);
    DWORD result = WaitForSingleObject((HANDLE)semaphore.v[0], wait_ms);
    return (result == WAIT_OBJECT_0);
}

void os_semaphore_drop(Semaphore semaphore) {
    if (MemoryIsZeroStruct(&semaphore)) {
        return;
    }
    ReleaseSemaphore((HANDLE)semaphore.v[0], 1, 0);
}

Barrier os_barrier_alloc(u64 count) {
    OS_W32_Entity *entity = os_w32_entity_alloc(OS_W32_Entity_Kind_Barrier);
    entity->barrier.count = 0;
    entity->barrier.trip_count = count;
    InitializeCriticalSection(&entity->barrier.mutex);
    InitializeConditionVariable(&entity->barrier.cond);
    Barrier result = {(u64)entity};
    return result;
}

void os_barrier_release(Barrier barrier) {
    if (MemoryIsZeroStruct(&barrier)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)barrier.v[0];
    DeleteCriticalSection(&entity->barrier.mutex);
    os_w32_entity_release(entity);
}

void os_barrier_wait(Barrier barrier) {
    if (MemoryIsZeroStruct(&barrier)) {
        return;
    }
    OS_W32_Entity *entity = (OS_W32_Entity *)barrier.v[0];
    EnterCriticalSection(&entity->barrier.mutex);
    entity->barrier.count += 1;
    if (entity->barrier.count >= entity->barrier.trip_count) {
        entity->barrier.count = 0;
        WakeAllConditionVariable(&entity->barrier.cond);
        LeaveCriticalSection(&entity->barrier.mutex);
    } else {
        SleepConditionVariableCS(&entity->barrier.cond, &entity->barrier.mutex, INFINITE);
        LeaveCriticalSection(&entity->barrier.mutex);
    }
}

OS_Handle os_library_open(String path) {
    Scratch   scratch = arena_get_scratch(0, 0);
    String16  path16 = str16_from_str(scratch.arena, path);
    HMODULE   lib = LoadLibraryW((WCHAR *)path16.str);
    OS_Handle result = {(u64)lib};
    arena_end_scratch(&scratch);
    return result;
}

void *os_library_load_proc(OS_Handle lib, String name) {
    Scratch scratch = arena_get_scratch(0, 0);
    String  name_copy = str_push_copy(scratch.arena, name);
    void   *proc = (void *)GetProcAddress((HMODULE)lib.v[0], (char *)name_copy.str);
    arena_end_scratch(&scratch);
    return proc;
}

void os_library_close(OS_Handle lib) {
    if (os_handle_match(lib, os_handle_zero())) {
        return;
    }
    FreeLibrary((HMODULE)lib.v[0]);
}

void os_safe_call(Thread_Entry_Point *func, Thread_Entry_Point *fail_handler, void *ptr) {
    __try {
        func(ptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (fail_handler) {
            fail_handler(ptr);
        }
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
    CoCreateGuid((GUID *)&guid);
    return guid;
}

typedef struct Cmd_Line Cmd_Line;
void                    entry_point(Cmd_Line *cmd_line);

static void win32_entry_point(int argc, WCHAR **wargv) {
    {
        OS_System_Info *info = &os_w32_state.system_info;
        SYSTEM_INFO     sys_info;
        GetSystemInfo(&sys_info);
        info->logical_processors = sys_info.dwNumberOfProcessors;
        info->page_size = sys_info.dwPageSize;

        LARGE_INTEGER perf_freq;
        QueryPerformanceFrequency(&perf_freq);
        os_w32_state.counts_per_second = perf_freq.QuadPart;
    }

    {
        OS_Process_Info *info = &os_w32_state.process_info;
        info->pid = GetCurrentProcessId();
    }

    CoInitializeEx(0, COINIT_MULTITHREADED);

    os_w32_state.arena = arena_alloc();
    os_w32_state.entity_arena = arena_alloc();
    InitializeCriticalSection(&os_w32_state.entity_mutex);

    {
        Scratch          scratch = arena_get_scratch(0, 0);
        OS_Process_Info *info = &os_w32_state.process_info;

        {
            DWORD    size = KB(32);
            u16     *buffer = push_array_no_zero(scratch.arena, u16, size);
            DWORD    length = GetModuleFileNameW(0, (WCHAR *)buffer, size);
            String16 name16 = {buffer, length};
            String   name = str_from_str16(scratch.arena, name16);
            String   name_chopped = str_chop_last_slash(name);
            info->binary_path = str_push_copy(os_w32_state.arena, name_chopped);
        }

        info->initial_path = os_get_current_path(os_w32_state.arena);

        {
            u16 buffer[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, (WCHAR *)buffer))) {
                String16 path16 = str16_cstring(buffer);
                info->user_program_data_path = str_from_str16(os_w32_state.arena, path16);
            }
        }

        arena_end_scratch(&scratch);
    }

    {
        TCTX *main_tctx = tctx_alloc();
        tctx_select(main_tctx);
        tctx_set_thread_name(str_lit("Main Thread"));
    }

    Scratch     scratch = arena_get_scratch(0, 0);
    String_List args_list = {0};
    for (int i = 0; i < argc; i++) {
        String16 arg16 = str16_cstring((u16 *)wargv[i]);
        String   arg = str_from_str16(scratch.arena, arg16);
        str_list_push(scratch.arena, &args_list, arg);
    }
    Cmd_Line cmd_line = cmd_line_from_string_list(scratch.arena, args_list);

    entry_point(&cmd_line);

    arena_end_scratch(&scratch);
}

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    win32_entry_point(__argc, __wargv);
    return 0;
}
