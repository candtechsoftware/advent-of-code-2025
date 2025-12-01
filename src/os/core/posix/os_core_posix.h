#pragma once

#include "../../../base/base_inc.h"
#include "../../os.h"
#include "../../os_thread.h"

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#if OS_LINUX
#    include <features.h>
#    include <linux/limits.h>
#    include <sys/sysinfo.h>
#    include <sys/sendfile.h>
pid_t gettid(void);
int   pthread_setname_np(pthread_t thread, const char *name);
int   pthread_getname_np(pthread_t thread, char *name, size_t size);
#elif OS_MAC
#    include <sys/sysctl.h>
#    include <sys/param.h>
#    include <mach/mach_time.h>
#    include <mach-o/dyld.h>
#    include <copyfile.h>
#    include <dispatch/dispatch.h>
#    ifdef __OBJC__
#        import <Foundation/Foundation.h>
#    endif
#endif

typedef struct tm       tm;
typedef struct timespec timespec;

typedef struct OS_Posix_File_Iter OS_Posix_File_Iter;
struct OS_Posix_File_Iter {
    DIR           *dir;
    struct dirent *dp;
    String         path;
};

StaticAssert(sizeof(Member(OS_File_Iter, memory)) >= sizeof(OS_Posix_File_Iter), os_posix_file_iter_size_check);

typedef struct OS_Posix_Safe_Call_Chain OS_Posix_Safe_Call_Chain;
struct OS_Posix_Safe_Call_Chain {
    OS_Posix_Safe_Call_Chain *next;
    Thread_Entry_Point       *fail_handler;
    void                     *ptr;
};
thread_static OS_Posix_Safe_Call_Chain *os_posix_safe_call_chain = 0;

typedef enum OS_Posix_Entity_Kind {
    OS_Posix_Entity_Kind_Thread,
    OS_Posix_Entity_Kind_Mutex,
    OS_Posix_Entity_Kind_RW_Mutex,
    OS_Posix_Entity_Kind_Condition_Var,
    OS_Posix_Entity_Kind_Barrier,
} OS_Posix_Entity_Kind;

typedef struct OS_Posix_Entity OS_Posix_Entity;
struct OS_Posix_Entity {
    OS_Posix_Entity     *next;
    OS_Posix_Entity_Kind kind;
    union {
        struct {
            pthread_t           handle;
            Thread_Entry_Point *func;
            void               *ptr;
        } thread;
        pthread_mutex_t  mutex_handle;
        pthread_rwlock_t rw_mutex_handle;
        struct {
            pthread_cond_t  cond_handle;
            pthread_mutex_t rwlock_mutex_handle;
        } cv;
#if OS_LINUX
        pthread_barrier_t barrier;
#elif OS_MAC
        struct {
            u64 count;
            u64 trip_count;

            pthread_mutex_t mutex;
            pthread_cond_t  cond;
        } barrier;
#endif
    };
};

typedef struct OS_Posix_State OS_Posix_State;
struct OS_Posix_State {
    Arena           *arena;
    OS_Process_Info  process_info;
    OS_System_Info   system_info;
    pthread_mutex_t  entity_mutex;
    Arena           *entity_arena;
    OS_Posix_Entity *entity_free;
};

static OS_Posix_State os_posix_state = {0};

static Date_Time       os_posix_date_time_from_tm(tm in, u32 msec);
static tm              os_posix_tm_from_date_time(Date_Time dt);
static timespec        os_posix_timespec_from_date_time(Date_Time dt);
static Dense_Time      os_posix_dense_time_from_timespec(timespec in);
static File_Properties os_posix_file_properties_from_state(struct stat *s);
static void            os_posix_safe_call_sig_handler(int x);

static OS_Posix_Entity *os_posix_entity_alloc(OS_Posix_Entity_Kind kind);
static void             os_posix_entity_release(OS_Posix_Entity *entity);

static void *os_posix_thread_entry_point(void *ptr);
