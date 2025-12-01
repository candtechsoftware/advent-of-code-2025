#include "base_thread.h"
#include "../os/os_inc.h"

Thread
thread_launch(Thread_Entry_Point *f, void *p) {
    Thread result = os_thread_launch(f, p);
    return result;
}

b32 thread_join(Thread thread, u64 endt_us) {
    b32 result = os_thread_join(thread, endt_us);
    return result;
}

void thread_detach(Thread thread) {
    os_thread_detach(thread);
}

Mutex mutex_alloc(void) {
    Mutex result = os_mutex_alloc();
    return result;
}

void mutex_release(Mutex mutex) {
    os_mutex_release(mutex);
}

void mutex_take(Mutex mutex) {
    os_mutex_take(mutex);
}

void mutex_drop(Mutex mutex) {
    os_mutex_drop(mutex);
}

RW_Mutex
rw_mutex_alloc(void) {
    RW_Mutex result = os_rw_mutex_alloc();
    return result;
}

void 
rw_mutex_release(RW_Mutex mutex) {
    os_rw_mutex_release(mutex);
}

void 
rw_mutex_take(RW_Mutex mutex, b32 write_mode) {
    os_rw_mutex_take(mutex, write_mode);
}

void 
rw_mutex_drop(RW_Mutex mutex, b32 write_mode) {
    os_rw_mutex_drop(mutex, write_mode);
}

Cond_Var
cond_var_alloc(void) {
    Cond_Var result = os_cond_var_alloc();
    return result;
}

void cond_var_release(Cond_Var cv) {
    os_cond_var_release(cv);
}

b32 cond_var_wait(Cond_Var cv, Mutex mutex, u64 endt_us) {
    b32 result = os_cond_var_wait(cv, mutex, endt_us);
    return result;
}

b32 cond_var_wait_rw(Cond_Var cv, RW_Mutex mutex_rw, b32 write_mode, u64 endt_us) {
    b32 result = os_cond_var_wait_rw(cv, mutex_rw, write_mode, endt_us);
    return result;
}

void cond_var_signal(Cond_Var cv) {
    os_cond_var_signal(cv);
}

void cond_var_broadcast(Cond_Var cv) {
    os_cond_var_broadcast(cv);
}

Semaphore
semaphore_alloc(u32 initial_count, u32 max_count, String name) {
    Semaphore result = os_semaphore_alloc(initial_count, max_count, name);
    return result;
}

void semaphore_release(Semaphore semaphore) {
    os_semaphore_release(semaphore);
}

Semaphore
semaphore_open(String name) {
    Semaphore result = os_semaphore_open(name);
    return result;
}

void semaphore_close(Semaphore semaphore) {
    os_semaphore_close(semaphore);
}

b32 semaphore_take(Semaphore semaphore, u64 endt_us) {
    b32 result = os_semaphore_take(semaphore, endt_us);
    return result;
}

void semaphore_drop(Semaphore semaphore) {
    os_semaphore_drop(semaphore);
}

Barrier
barrier_alloc(u64 count) {
    Barrier result = os_barrier_alloc(count);
    return result;
}

void barrier_release(Barrier barrier) {
    os_barrier_release(barrier);
}

void barrier_wait(Barrier barrier) {
    os_barrier_wait(barrier);
}

Stripe_Array
stripe_array_alloc(Arena *arena) {
    Stripe_Array    result = {0};
    OS_System_Info *info = os_get_system_info();
    u64             count = info->logical_processors;
    result.count = count;
    result.v = push_array(arena, Stripe, count);
    for (u64 i = 0; i < count; i += 1) {
        result.v[i].arena = arena_alloc();
        result.v[i].rw_mutex = rw_mutex_alloc();
        result.v[i].cond_var = cond_var_alloc();
    }
    return result;
}

void stripe_array_release(Stripe_Array *stripes) {
    for (u64 i = 0; i < stripes->count; i += 1) {
        arena_release(stripes->v[i].arena);
        rw_mutex_release(stripes->v[i].rw_mutex);
        cond_var_release(stripes->v[i].cond_var);
    }
    MemoryZeroStruct(stripes);
}

Stripe *
stripe_from_slot_idx(Stripe_Array *stripes, u64 slot_index) {
    Stripe *result = 0;
    if (stripes->count > 0) {
        result = &stripes->v[slot_index % stripes->count];
    }
    return result;
}
