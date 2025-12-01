#pragma once

#include "../base/base.h"
#include "../base/arena.h"

typedef struct Thread Thread;
struct Thread {
    u64 v[1];
};

typedef void Thread_Entry_Point(void *p);

typedef struct Mutex Mutex;
struct Mutex {
    u64 v[1];
};

typedef struct RW_Mutex RW_Mutex;
struct RW_Mutex {
    u64 v[1];
};

typedef struct Cond_Var Cond_Var;
struct Cond_Var {
    u64 v[1];
};

typedef struct Semaphore Semaphore;
struct Semaphore {
    u64 v[1];
};

typedef struct Barrier Barrier;
struct Barrier {
    u64 v[1];
};

typedef struct Stripe Stripe;
struct Stripe {
    Arena   *arena;
    RW_Mutex rw_mutex;
    Cond_Var cond_var;
    void    *free;
};

typedef struct Stripe_Array Stripe_Array;
struct Stripe_Array {
    Stripe *v;
    u64     count;
};

#define rw_mutex_take_r(m) rw_mutex_take((m), (0))
#define rw_mutex_take_w(m) rw_mutex_take((m), (1))
#define rw_mutex_drop_r(m) rw_mutex_drop((m), (0))
#define rw_mutex_drop_w(m) rw_mutex_drop((m), (1))

#define cond_var_wait_rw_r(cv, m, endt) cond_var_wait_rw((cv), (m), (0), (endt))
#define cond_var_wait_rw_w(cv, m, endt) cond_var_wait_rw((cv), (m), (1), (endt))

#define MutexScopeRWPromote(m) DeferLoop((rw_mutex_drop_r(m), rw_mutex_take_w(m)), (rw_mutex_drop_w(m), rw_mutex_take_r(m)))
