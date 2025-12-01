#pragma once

#include "../os/os_thread.h"
#include "string_core.h"

Thread thread_launch(Thread_Entry_Point *f, void *p);
b32    thread_join(Thread thread, u64 endt_us);
void   thread_detach(Thread thread);

Mutex mutex_alloc(void);
void  mutex_release(Mutex mutex);
void  mutex_take(Mutex mutex);
void  mutex_drop(Mutex mutex);

RW_Mutex rw_mutex_alloc(void);
void     rw_mutex_release(RW_Mutex mutex);
void     rw_mutex_take(RW_Mutex mutex, b32 write_mode);
void     rw_mutex_drop(RW_Mutex mutex, b32 write_mode);

Cond_Var cond_var_alloc(void);
void     cond_var_release(Cond_Var cv);
b32      cond_var_wait(Cond_Var cv, Mutex mutex, u64 endt_us);
b32      cond_var_wait_rw(Cond_Var cv, RW_Mutex mutex_rw, b32 write_mode, u64 endt_us);
void     cond_var_signal(Cond_Var cv);
void     cond_var_broadcast(Cond_Var cv);

Semaphore semaphore_alloc(u32 initial_count, u32 max_count, String name);
void      semaphore_release(Semaphore semaphore);
Semaphore semaphore_open(String name);
void      semaphore_close(Semaphore semaphore);
b32       semaphore_take(Semaphore semaphore, u64 endt_us);
void      semaphore_drop(Semaphore semaphore);

Barrier barrier_alloc(u64 count);
void    barrier_release(Barrier barrier);
void    barrier_wait(Barrier barrier);

Stripe_Array stripe_array_alloc(Arena *arena);
void         stripe_array_release(Stripe_Array *stripes);
Stripe      *stripe_from_slot_idx(Stripe_Array *stripes, u64 slot_index);

#define MutexScope(m)  DeferLoop(mutex_take(m), mutex_drop(m))
#define MutexScopeW(m) DeferLoop(rw_mutex_take_w(m), rw_mutex_drop_w(m))
#define MutexScopeR(m) DeferLoop(rw_mutex_take_r(m), rw_mutex_drop_r(m))
