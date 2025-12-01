#pragma once

#include "../../../base/base_inc.h"
#include "../../os.h"
#include "../../os_thread.h"

#include <winsock2.h>
#include <mswsock.h>
#include <Windows.h>
#include <intrin.h>
#include <windows.h>
#include <windowsx.h>
#include <timeapi.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <processthreadsapi.h>

typedef enum OS_W32_Entity_Kind {
    OS_W32_Entity_Kind_Thread,
    OS_W32_Entity_Kind_Mutex,
    OS_W32_Entity_Kind_RW_Mutex,
    OS_W32_Entity_Kind_ConditionVariable,
    OS_W32_Entity_Kind_Barrier,
} OS_W32_Entity_Kind;

typedef struct OS_W32_Entity OS_W32_Entity;
struct OS_W32_Entity {
    OS_W32_Entity     *next;
    OS_W32_Entity_Kind kind;
    union {
        struct {
            HANDLE              handle;
            DWORD               tid;
            Thread_Entry_Point *func;
            void               *ptr;
        } thread;
        CRITICAL_SECTION mutex_handle;
        SRWLOCK          rw_mutex_handle;
        struct {
            CONDITION_VARIABLE cond_handle;
            CRITICAL_SECTION   mutex_handle;
        } cv;
        struct {
            u64                count;
            u64                trip_count;
            CRITICAL_SECTION   mutex;
            CONDITION_VARIABLE cond;
        } barrier;
    };
};

typedef struct OS_W32_File_Iter OS_W32_File_Iter;
struct OS_W32_File_Iter {
    HANDLE           handle;
    WIN32_FIND_DATAW find_data;
    String           path;
    b32              first;
};

StaticAssert(sizeof(Member(OS_File_Iter, memory)) >= sizeof(OS_W32_File_Iter), os_w32_file_iter_size_check);

typedef struct OS_W32_State OS_W32_State;
struct OS_W32_State {
    Arena           *arena;
    OS_Process_Info  process_info;
    OS_System_Info   system_info;
    CRITICAL_SECTION entity_mutex;
    Arena           *entity_arena;
    OS_W32_Entity   *entity_free;
    u64              process_start_time;
    u64              counts_per_second;
};

static OS_W32_State os_w32_state = {0};

static OS_W32_Entity *os_w32_entity_alloc(OS_W32_Entity_Kind kind);
static void           os_w32_entity_release(OS_W32_Entity *entity);

static DWORD WINAPI os_w32_thread_entry_point(void *ptr);

static File_Properties os_w32_file_properties_from_find_data(WIN32_FIND_DATAW *fd);
static File_Properties os_w32_file_properties_from_handle(HANDLE handle);
