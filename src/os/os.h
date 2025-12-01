#pragma once
#include "../base/base.h"
#include "../base/arena.h"
#include "../base/string_core.h"
#include "../base/math.h"
#include "os_thread.h"

typedef struct OS_System_Info OS_System_Info;
struct OS_System_Info {
    u32 logical_processors;
    u64 page_size;
};

typedef u32 Data_Access_Flags;
enum {
    Data_Access_Flag_Read = (1 << 0),
    Data_Access_Flag_Write = (1 << 1),
    Data_Access_Flag_Execute = (1 << 2),
};

typedef u32 File_Property_Flags;
enum {
    File_Property_Flag_Is_Folder = (1 << 0),
};

typedef struct File_Properties {
    String              name;
    u64                 size;
    File_Property_Flags flags;
    Dense_Time          create_time;
    Dense_Time          modify_time;
    Data_Access_Flags   access;
} File_Properties;

typedef struct OS_Process_Info OS_Process_Info;
struct OS_Process_Info {
    u32         pid;
    b32         large_page_allowed;
    String      binary_path;
    String      initial_path;
    String      user_program_data_path;
    String_List module_load_paths;
    String_List env;
};

typedef u32 OS_Access_Flags;
enum {
    OS_Access_Flag_Read = (1 << 0),
    OS_Access_Flag_Write = (1 << 1),
    OS_Access_Flag_Execute = (1 << 2),
    OS_Access_Flag_Append = (1 << 3),
    OS_Access_Flag_Share_Read = (1 << 4),
    OS_Access_Flag_Share_Write = (1 << 4),
    OS_Access_Flag_Inherited = (1 << 6),
};

typedef u32 OS_Filter_Iter_Flags;
enum {
    OS_Filter_Iter_Flag_Skip_Folders = (1 << 0),
    OS_Filter_Iter_Flag_Skip_Files = (1 << 1),
    OS_Filter_Iter_Flag_Skip_Hidden = (1 << 2),
    OS_Filter_Iter_Flag_Done = (1 << 31),
};

typedef struct OS_File_Iter OS_File_Iter;
struct OS_File_Iter {
    OS_Filter_Iter_Flags flags;
    u8                   memory[800];
};

typedef struct OS_File_ID OS_File_ID;
struct OS_File_ID {
    u64 v[3];
};

typedef struct OS_Handle OS_Handle;
struct OS_Handle {
    u64 v[1];
};

typedef struct OS_Handle_Node OS_Handle_Node;
struct OS_Handle_Node {
    OS_Handle_Node *next;
    OS_Handle       v;
};

typedef struct OS_Handle_List OS_Handle_List;
struct OS_Handle_List {
    OS_Handle_Node *first;
    OS_Handle_Node *last;
    u64             count;
};

typedef struct OS_Handle_Array OS_Handle_Array;
struct OS_Handle_Array {
    OS_Handle *v;
    u64        count;
};

typedef struct OS_Process_Launch_Params OS_Process_Launch_Params;
struct OS_Process_Launch_Params {
    String_List cmd_line;
    String      path;
    String_List env;
    b32         inherit_env;
    b32         debug_process;
    b32         consoleless;
    OS_Handle   stdout_file;
    OS_Handle   stderr_file;
    OS_Handle   stdin_file;
};

static inline OS_Handle os_handle_zero(void);
static inline b32       os_handle_match(OS_Handle a, OS_Handle b);
void                    os_handle_list_push(Arena *arena, OS_Handle_List *handles, OS_Handle handle);
OS_Handle_Array         os_handle_array_from_list(Arena *arena, OS_Handle_List list);

String      os_data_from_file_path(Arena *arena, String path);
b32         os_write_data_to_file(String path, String data);
b32         os_write_data_list_to_file_path(String path, String_List list);
b32         os_append_data_to_file_path(String path, String data);
OS_File_ID  os_id_from_file_path(String path);
s64         os_file_id_compare(OS_File_ID a, OS_File_ID b);
String      os_string_from_file_range(Arena *arena, OS_Handle file, Rng1_u64 range);
String      os_file_read_cstring(Arena *arena, OS_Handle file, u64 off);
String_List os_string_from_argcv(Arena *arena, int argc, char **argv);

OS_Handle os_cmd_line_launch(String string);
OS_Handle os_cmd_line_launchf(char *fmt, ...);

OS_System_Info  *os_get_system_info(void);
OS_Process_Info *os_get_process_info(void);
String           os_get_current_path(Arena *arena);
u32              os_get_process_start_time_unix(void);
u64              os_now_microseconds(void);

void *os_reserve(u64 size);
b32   os_commit(void *ptr, u64 size);
void  os_decommit(void *ptr, u64 size);
void  os_mem_release(void *ptr, u64 size);

void *os_reserve_large(u64 size);
b32   os_commit_large(void *ptr, u64 size);

u32  os_tid(void);
void os_set_thread_name(String string);

void os_abort(s32 exit_code);

OS_Handle os_file_open(OS_Access_Flags flags, String path);
void      os_file_close(OS_Handle file);
u64       os_file_read(OS_Handle file, Rng1_u64 rn, void *out_data);

#define os_file_read_struct(f, off, ptr) os_file_read((f), r1u64((off), (off) + sizeof(*(ptr))), (ptr))
u64             os_file_write(OS_Handle file, Rng1_u64 rng, void *data);
b32             os_file_set_times(OS_Handle file, Date_Time time);
File_Properties os_properties_from_file(OS_Handle file);
OS_File_ID      os_id_from_file(OS_Handle file);
b32             os_file_reserve_size(OS_Handle file, u64 size);
b32             os_delete_file_at_path(String path);
b32             os_copy_file_path(String dst, String src);
b32             os_move_file_path(String dst, String src);
String          os_full_path_from_path(Arena *arena, String path);
b32             os_file_path_exists(String path);
b32             os_folder_path_exists(String path);
File_Properties os_properties_from_file_path(String path);

OS_Handle os_file_map_open(OS_Access_Flags flags, OS_Handle file);
void      os_file_map_close(OS_Handle map);
void     *os_file_map_view_open(OS_Handle map, OS_Access_Flags flags, Rng1_u64 range);
void      os_file_map_view_close(OS_Handle map, void *ptr, Rng1_u64 range);

OS_File_Iter *os_file_iter_begin(Arena *arena, String path, OS_Filter_Iter_Flags flags);
b32           os_file_iter_next(Arena *arena, OS_File_Iter *iter, File_Properties *info_out);
void          os_file_iter_end(OS_File_Iter *iter);

b32 os_make_directory(String path);

OS_Handle os_shared_memory_alloc(u64 size, String name);
OS_Handle os_shared_memory_open(String name);
void      os_shared_memory_close(OS_Handle handle);
void     *os_shared_memory_view_open(OS_Handle handle, Rng1_u64 range);
void      os_shared_memory_view_close(OS_Handle handle, void *ptr, Rng1_u64 range);

Thread os_thread_launch(Thread_Entry_Point *func, void *ptr);
b32    os_thread_join(Thread thread, u64 endt_us);
void   os_thread_detach(Thread thread);

Mutex os_mutex_alloc(void);
void  os_mutex_release(Mutex mutex);
void  os_mutex_take(Mutex mutex);
void  os_mutex_drop(Mutex mutex);

RW_Mutex os_rw_mutex_alloc(void);
void     os_rw_mutex_release(RW_Mutex mutex);
void     os_rw_mutex_take(RW_Mutex mutex, b32 write_mode);
void     os_rw_mutex_drop(RW_Mutex mutex, b32 write_mode);

Cond_Var os_cond_var_alloc(void);
void     os_cond_var_release(Cond_Var cv);
b32      os_cond_var_wait(Cond_Var cv, Mutex mutex, u64 endt_us);
b32      os_cond_var_wait_rw(Cond_Var cv, RW_Mutex mutex_rw, b32 write_mode, u64 endt_us);
void     os_cond_var_signal(Cond_Var cv);
void     os_cond_var_broadcast(Cond_Var cv);

Semaphore os_semaphore_alloc(u32 initial_count, u32 max_count, String name);
void      os_semaphore_release(Semaphore semaphore);
Semaphore os_semaphore_open(String name);
void      os_semaphore_close(Semaphore semaphore);
b32       os_semaphore_take(Semaphore semaphore, u64 endt_us);
void      os_semaphore_drop(Semaphore semaphore);

Barrier os_barrier_alloc(u64 count);
void    os_barrier_release(Barrier barrier);
void    os_barrier_wait(Barrier barrier);
