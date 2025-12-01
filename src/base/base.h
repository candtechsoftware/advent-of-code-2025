#pragma once

#if defined(__gnu_linux__) || defined(__linux__)
#    ifndef _GNU_SOURCE
#        define _GNU_SOURCE
#    endif
#endif

#if defined(__AVX2__)
    #define USE_AVX2 1
    #include <immintrin.h>
    #define SIMD_WIDTH 8
    #define SIMD_ALIGN 32

#elif defined(__SSE4_1__)
    #define USE_SSE4 1
    #include <smmintrin.h>
    #define SIMD_WIDTH 4
    #define SIMD_ALIGN 16

#elif defined(__ARM_NEON) || defined(__aarch64__)
    #define USE_NEON 1
    #include <arm_neon.h>
    #define SIMD_WIDTH 4
    #define SIMD_ALIGN 16

#else
    #define USE_SCALAR 1
    #define SIMD_WIDTH 1
    #define SIMD_ALIGN 4
#endif

#define MAX_MOVES 100000

#define SENTINEL(type) ((type) - 1)

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef s8        b8;
typedef s16       b16;
typedef s32       b32;
typedef s64       b64;
typedef float     f32;
typedef double    f64;
typedef uintptr_t UAddr;
typedef struct {
    u64 lo, hi;
} u128;

#define MIN_S8  ((s8)0x80)
#define MIN_S16 ((s16)0x8000)
#define MIN_S32 ((s32)0x80000000)
#define MIN_S64 ((s64)0x8000000000000000llu)

#define MAX_S8  ((s8)0x7f)
#define MAX_S16 ((s16)0x7fff)
#define MAX_S32 ((s32)0x7fffffff)
#define MAX_S64 ((s64)0x7fffffffffffffffllu)

#define MAX_U8  0xff
#define MAX_U16 0xffff
#define MAX_U32 0xffffffff
#define MAX_U64 0xffffffffffffffffllu

#if defined(__clang__)
#    define COMPILER_CLANG 1
#elif defined(__GNUC__)
#    define COMPILER_GCC 1
#elif defined(_MSC_VER)
#    define COMPILER_MSVC 1
#endif

#if defined(__clang__) || defined(__GNUC__)
#    if defined(_WIN32)
#        define OS_WINDOWS 1
#    elif defined(__gnu_linux__)
#        define OS_LINUX 1
#    elif defined(__APPLE__) && defined(__MACH__)
#        define OS_MAC 1
#    else
#        error missing OS detection
#    endif

#    if defined(__amd64__) || defined(__x86_64__)
#        define ARCH_X64 1
#    elif defined(__i386__)
#        define ARCH_X86 1
#    elif defined(__arm__)
#        define ARCH_ARM 1
#    elif defined(__aarch64__)
#        define ARCH_ARM64 1
#    else
#        error missing ARCH detection
#    endif
#elif defined(_MSC_VER)
#    if defined(_WIN32) || defined(_WIN64)
#        define OS_WINDOWS 1
#    endif
#    if defined(_M_AMD64) || defined(_M_X64)
#        define ARCH_X64 1
#    elif defined(_M_IX86)
#        define ARCH_X86 1
#    elif defined(_M_ARM)
#        define ARCH_ARM 1
#    elif defined(_M_ARM64)
#        define ARCH_ARM64 1
#    endif
#endif

#define BASE_UNUSED(x)        (void)x
#define BASE_UNUSED_RESULT(x) (void)x

#if !defined(RUN_MODE_DEBUG) && !defined(RUN_MODE_PROFILE) && !defined(RUN_MODE_OPTIMIZED) && !defined(RUN_MODE_DEBUG_PROFILE)
#    define RUN_MODE_DEBUG 1
#endif

#if defined(__APPLE__)
#    define read_only __attribute__((section("__TEXT,__const")))
#elif defined(__GNUC__) || defined(__clang__)
#    define read_only __attribute__((section(".rodata")))
#elif defined(_MSC_VER)
#    define read_only __declspec(allocate(".rdata"))
#else
#    define read_only
#endif

#if defined(__GNUC__) || defined(__clang__)
#    define thread_static __thread
#elif defined(_MSC_VER)
#    define thread_static __declspec(thread)
#else
#    define thread_static
#endif

#if defined(RUN_MODE_DEBUG)
#    define DEBUG_MODE     1
#    define PROFILE_MODE   0
#    define OPTIMIZED_MODE 0
#elif defined(RUN_MODE_PROFILE)
#    define DEBUG_MODE     0
#    define PROFILE_MODE   1
#    define OPTIMIZED_MODE 0
#elif defined(RUN_MODE_OPTIMIZED)
#    define DEBUG_MODE     0
#    define PROFILE_MODE   0
#    define OPTIMIZED_MODE 1
#elif defined(RUN_MODE_DEBUG_PROFILE)
#    define DEBUG_MODE     1
#    define PROFILE_MODE   1
#    define OPTIMIZED_MODE 0
#endif

#if DEBUG_MODE
#    define BUILD_DEBUG 1
#else
#    define BUILD_DEBUG 0
#endif

#if LANG_CPP
#    define C_LINKAGE_BEGIN extern "C" {
#    define C_LINKAGE_END   }
#    define C_LINKAGE       extern "C"
#else
#    define C_LINKAGE_BEGIN
#    define C_LINKAGE_END
#    define C_LINKAGE
#endif

#if COMPILER_CLANG || COMPILER_GCC
#    define THREAD_VAR __thread
#elif COMPILER_MSVC
#    define THREAD_VAR __declspec(thread)
#else
#    error "Unknown compiler for THREAD_VAR"
#endif

#if PROFILE_MODE
#    define ProfOpen(n)       prof_open((char *)(n))
#    define ProfClose()       prof_close()
#    define ProfThreadBegin() prof_thread_begin()
#    define ProfThreadEnd()   prof_thread_end()
#    define ProfThreadFlush() prof_thread_flush()
#    define ProfBegin(s)      prof_begin((s).str, (s).size)
#    define ProfEnd()         prof_end()
#else
#    define ProfOpen(n)
#    define ProfClose()
#    define ProfThreadBegin()
#    define ProfThreadEnd()
#    define ProfThreadFlush()
#    define ProfBegin(s)
#    define ProfEnd()
#endif

#if PROFILE_MODE
#    define ProfBeginStr(n) ProfBegin(str_lit(n))
#    define ProfEndManual() ProfEnd()
#    define ProfBeginFunc() prof_begin((const u8 *)__FUNCTION__, sizeof(__FUNCTION__) - 1)
#    define ProfEndFunc()   ProfEnd()
#else
#    define ProfBeginStr(n)
#    define ProfEndManual()
#    define ProfBeginFunc()
#    define ProfEndFunc()
#endif

#if ARCH_X64 || ARCH_ARM64
#    define ARCH_ADDRSIZE 64
#else
#    define ARCH_ADDRSIZE 32
#endif

#if COMPILER_MSVC
#    if defined(__SANITIZE_ADDRESS__)
#        define ASAN_ENABLED 1
#        define NO_ASAN      __declspec(no_sanitize_address)
#    else
#        define NO_ASAN
#    endif
#elif COMPILER_CLANG
#    if defined(__has_feature)
#        if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#            define ASAN_ENABLED 1
#        endif
#    endif
#    define NO_ASAN __attribute__((no_sanitize("address")))
#elif COMPILER_GCC
#    if defined(__SANITIZE_ADDRESS__)
#        define ASAN_ENABLED 1
#    endif
#    define NO_ASAN __attribute__((no_sanitize_address))
#else
#    define NO_ASAN
#endif

#if ASAN_ENABLED
C_LINKAGE void __asan_poison_memory_region(void const volatile *addr, size_t size);
C_LINKAGE void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#    define AsanPoisonMemoryRegion(addr, size)   __asan_poison_memory_region((addr), (size))
#    define AsanUnpoisonMemoryRegion(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
#    define AsanPoisonMemoryRegion(addr, size)   ((void)(addr), (void)(size))
#    define AsanUnpoisonMemoryRegion(addr, size) ((void)(addr), (void)(size))
#endif

#if ASAN_ENABLED
#    define AsanPoison(p, z)   AsanPoisonMemoryRegion((p), (z))
#    define AsanUnpoison(p, z) AsanUnpoisonMemoryRegion((p), (z))
#else
#    define AsanPoison(p, z)
#    define AsanUnpoison(p, z)
#endif

#define Stmnt(S) \
    do {         \
        S        \
    } while (0)

#define Glue_(A, B) A##B
#define Glue(A, B)  Glue_(A, B)

#if !defined(AssertBreak)
#    define AssertBreak() (*(volatile int *)0 = 0)
#endif

#if DEBUG_MODE
#    define Assert(c) Stmnt(if (!(c)) { AssertBreak(); })
#else
#    define Assert(c)
#endif

#if defined(_MSC_VER)
#    define ALIGNAS(x) __declspec(align(x))
#elif defined(__GNUC__) || defined(__clang__)
#    define ALIGNAS(x) __attribute__((aligned(x)))
#else
#    error "Compiler does not support alignment attributes"
#endif

#define AssertAlways(c) Stmnt(if (!(c)) { AssertBreak(); })
#define InvalidPath     AssertAlways(!"InvalidPath")
#define NotImplemented  AssertAlways(!"NotImplemented")

#define DeferLoop(begin, end)        for (int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
#define DeferLoopChecked(begin, end) for (int _i_ = 2 * !(begin); (_i_ == 2 ? ((end), 0) : !_i_); _i_ += 1, (end))

#define EachIndex(it, count)   (u64 it = 0; (it) < (count); (it) += 1)
#define EachInRange(it, range) (u64 it = (range).min; it < (range).max; it += 1)
#define EachElement(it, array) (u64 it = 0; (it) < ArrayCount(array); (it) += 1)

static void memory_fill(void *ptr, u64 size, u8 fillbyte);
static b32  memory_match(void *a, void *b, u64 size);
static b32  memory_is_zero(void *ptr, u64 size);
#define StaticAssert(c, l) typedef u8 Glue(l, __LINE__)[(c) ? 1 : -1]

static inline void MemoryZero(void *p, u64 z) {
    memset(p, 0, z);
}
#define MemoryZeroStruct(p)   MemoryZero((p), sizeof(*(p)))
#define MemoryZeroArray(p)    MemoryZero((p), sizeof(p))
#define MemoryZeroTyped(m, c) MemoryZero((m), sizeof(*(m)) * (c))

static inline void *MemoryCopy(void *d, const void *s, u64 z) {
    return memmove(d, s, z);
}
#define MemoryCopyStruct(d, s) MemoryCopy((d), (s), Min(sizeof(*(d)), sizeof(*(s))))
#define MemoryCopyArray(d, s)  MemoryCopy((d), (s), Min(sizeof(s), sizeof(d)))

#define MemoryIsZero(p, z)    (memory_is_zero((p), (z)))
#define MemoryIsZeroStruct(p) (memory_is_zero((p), sizeof(*(p))))
#define MemoryIsZeroArray(p)  (memory_is_zero((p), sizeof(p)))

#define MemoryMatch(a, b, z)    (memory_match((a), (b), (z)))
#define MemoryMatchStruct(a, b) MemoryMatch((a), (b), sizeof(*(a)))
#define MemoryMatchArray(a, b)  MemoryMatch((a), (b), sizeof(a))

#define ArrayCount(a) (sizeof(a) / sizeof(*(a)))

#define IntFromPtr(p) (UAddr)(p)
#define PtrFromInt(n) (void *)((UAddr)(n))
#define PtrDif(a, b)  ((u8 *)(a) - (u8 *)(b))

#define Member(T, m)          (((T *)0)->m)
#define AddrOfMember(T, m)    (void *)(&Member(T, m))
#define OffsetOfMember(T, m)  IntFromPtr(&Member(T, m))
#define OffsetOfMemberV(s, m) PtrDif(&s->m, s)
#define MemberAddr(T, m)      AddrOfMember(T, m)
#define MemberOff(T, m)       OffsetOfMember(T, m)

#define Min(a, b)      (((a) < (b)) ? (a) : (b))
#define Max(a, b)      (((a) > (b)) ? (a) : (b))
#define Clamp(a, x, b) (((x) < (a)) ? (a) : ((b) < (x)) ? (b) \
                                                        : (x))
#define ClampTop(a, b) Min(a, b)
#define ClampBot(a, b) Max(a, b)

#define Swap(T, a, b) Stmnt(T t_ = (a); (a) = (b); (b) = t_;)

#define SignedIntFromCompare(a, b) (s32)(((b) < (a)) - ((a) < (b)))
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#    include <stdalign.h>
#    define ALIGNOF(type) alignof(type)
#elif defined(_MSC_VER)
#    define ALIGNOF(type) __alignof(type)
#elif defined(__GNUC__) || defined(__clang__)
#    define ALIGNOF(type) __alignof__(type)
#else
#    error "ALIGNOF not supported on this compiler"
#endif
#define AlignUpPow2(x, p)   (((x) + (p) - 1) & ~((p) - 1))
#define AlignDownPow2(x, p) ((x) & ~((p) - 1))
#define IsPow2OrZero(x)     (((x) & ((x) - 1)) == 0)

#define CeilIntDiv(n, d) (((n) + (d) - 1) / (d))

#define KB(x) ((u64)(x) << 10llu)
#define MB(x) ((u64)(x) << 20llu)
#define GB(x) ((u64)(x) << 30llu)
#define TB(x) ((u64)(x) << 40llu)

#define Thousand(x) ((x) * 1000llu)
#define Million(x)  ((x) * 1000000llu)
#define Billion(x)  ((x) * 1000000000llu)
#define Trillion(x) ((x) * 1000000000000llu)

#define inf_f32_as_u32     0x7f800000
#define neg_inf_f32_as_u32 0xff800000
#define inf_f64_as_u64     0x7ff0000000000000
#define neg_inf_f64_as_u64 0xfff0000000000000

#define set_inf_f32(x)                        \
    do {                                      \
        u32 _val = inf_f32_as_u32;            \
        MemoryCopy(&(x), &_val, sizeof(f32)); \
    } while (0)
#define set_neg_inf_f32(x)                    \
    do {                                      \
        u32 _val = neg_inf_f32_as_u32;        \
        MemoryCopy(&(x), &_val, sizeof(f32)); \
    } while (0)
#define set_inf_f64(x)                        \
    do {                                      \
        u64 _val = inf_f64_as_u64;            \
        MemoryCopy(&(x), &_val, sizeof(f64)); \
    } while (0)
#define set_neg_inf_f64(x)                    \
    do {                                      \
        u64 _val = neg_inf_f64_as_u64;        \
        MemoryCopy(&(x), &_val, sizeof(f64)); \
    } while (0)

#define DLLPushBack_NPZ(f, l, n, next, prev, nil)           \
    (((f) == (nil))                                         \
         ? ((f) = (l) = (n), (n)->next = (n)->prev = (nil)) \
         : ((n)->prev = (l), (l)->next = (n), (l) = (n), (n)->next = (nil)))

#define DLLPushBack(f, l, n)  DLLPushBack_NPZ(f, l, n, next, prev, 0)
#define DLLPushFront(f, l, n) DLLPushBack_NPZ(l, f, n, prev, next, 0)

#define DLLInsert_NPZ(f, l, p, n, next, prev, nil)                         \
    (((p) != (l))                                                          \
         ? ((n)->next = (p)->next, (n)->prev = (p), (p)->next->prev = (n), \
            (p)->next = (n))                                               \
         : ((n)->next = (nil), (n)->prev = (l), (l)->next = (n), (l) = (n)))

#define DLLInsert(f, l, p, n) DLLInsert_NPZ(f, l, p, n, next, prev, 0)

#define DLLRemove_NPZ(f, l, n, next, prev, nil)                         \
    ((f) == (n)   ? ((f) == (l) ? ((f) = (l) = (nil))                   \
                                : ((f) = (f)->next, (f)->prev = (nil))) \
     : (l) == (n) ? ((l) = (l)->prev, (l)->next = (nil))                \
                  : ((n)->next->prev = (n)->prev, (n)->prev->next = (n)->next))

#define DLLRemove(f, l, n) DLLRemove_NPZ(f, l, n, next, prev, 0)

#define SLLQueuePush_NZ(f, l, n, next, nil)                           \
    (((f) == (nil) ? (f) = (l) = (n) : ((l)->next = (n), (l) = (n))), \
     (n)->next = (nil))
#define SLLQueuePush(f, l, n) SLLQueuePush_NZ(f, l, n, next, 0)

#define SLLQueuePushFront_NZ(f, l, n, next, nil)         \
    ((f) == (nil) ? ((f) = (l) = (n), (n)->next = (nil)) \
                  : ((n)->next = (f), (f) = (n)))
#define SLLQueuePushFront(f, l, n) SLLQueuePushFront_NZ(f, l, n, next, 0)

#define SLLQueuePop_NZ(f, l, next, nil) \
    ((f) == (l) ? (f) = (l) = (nil) : ((f) = (f)->next))
#define SLLQueuePop(f, l) SLLQueuePop_NZ(f, l, next, 0)

#define SLLStackPush_N(f, n, next) ((n)->next = (f), (f) = (n))
#define SLLStackPush(f, n)         SLLStackPush_N(f, n, next)

#define SLLStackPop_NZ(f, next, nil) ((f) == (nil) ? (nil) : ((f) = (f)->next))
#define SLLStackPop(f)               SLLStackPop_NZ(f, next, 0)

#define min_S8  ((S8)0x80)
#define min_S16 ((S16)0x8000)
#define min_S32 ((S32)0x80000000)
#define min_S64 ((S64)0x8000000000000000llu)

#define max_S8  ((S8)0x7f)
#define max_S16 ((S16)0x7fff)
#define max_S32 ((S32)0x7fffffff)
#define max_S64 ((S64)0x7fffffffffffffffllu)

#define max_U8  0xff
#define max_U16 0xffff
#define max_U32 0xffffffff
#define max_U64 0xffffffffffffffffllu

#define machine_epsilon_F32 1.1920929e-7f
#define pi_F32              3.14159265359f
#define tau_F32             6.28318530718f
#define e_F32               2.71828182846f
#define gold_big_F32        1.61803398875f
#define gold_small_F32      0.61803398875f

#define machine_epsilon_F64 2.220446e-16
#define pi_F64              3.14159265359
#define tau_F64             6.28318530718
#define e_F64               2.71828182846
#define gold_big_F64        1.61803398875
#define gold_small_F64      0.61803398875

#define inf_F32_as_U32     0x7f800000
#define neg_inf_F32_as_U32 0xff800000
#define inf_F64_as_U64     0x7ff0000000000000
#define neg_inf_F64_as_U64 0xfff0000000000000

#if COMPILER_MSVC
#    include <intrin.h>
#    if ARCH_X64
#        define ins_atomic_u128_eval(x, r) \
            ((b32)InterlockedCompareExchange128((__int64 *)(x), 0, 0, (__int64 *)(r)))
#        define ins_atomic_u128_eval_cond_assign(x, k, c) (b32) InterlockedCompareExchange128((__int64 *)(x), ((__int64 *)&(k))[1], ((__int64 *)&(k))[0], (__int64 *)c)
#        define ins_atomic_u64_eval(x)                    *((volatile u64 *)(x))
#        define ins_atomic_u64_inc_eval(x)                InterlockedIncrement64((__int64 *)(x))
#        define ins_atomic_u64_dec_eval(x)                InterlockedDecrement64((__int64 *)(x))
#        define ins_atomic_u64_eval_assign(x, c)          InterlockedExchange64((__int64 *)(x), (c))
#        define ins_atomic_u64_add_eval(x, c)             InterlockedAdd64((__int64 *)(x), c)
#        define ins_atomic_u64_eval_cond_assign(x, k, c)  InterlockedCompareExchange64((__int64 *)(x), (k), (c))
#        define ins_atomic_u32_eval(x)                    *((volatile u32 *)(x))
#        define ins_atomic_u32_inc_eval(x)                InterlockedIncrement((LONG *)(x))
#        define ins_atomic_u32_dec_eval(x)                InterlockedDecrement((LONG *)(x))
#        define ins_atomic_u32_eval_assign(x, c)          InterlockedExchange((LONG *)(x), (c))
#        define ins_atomic_u32_eval_cond_assign(x, k, c)  InterlockedCompareExchange((LONG *)(x), (k), (c))
#        define ins_atomic_u32_add_eval(x, c)             InterlockedAdd((LONG *)(x), (c))
#        define ins_atomic_u8_eval_assign(x, c)           InterlockedExchange8((CHAR *)(x), (c))
#    else
#        error Atomic intrinsics not defined for this compiler / architecture combination.
#    endif
#elif COMPILER_CLANG || COMPILER_GCC
#    if ARCH_X64
#        define ins_atomic_u128_eval(x, r) \
            ({ __atomic_load((__int128 *)(x), (__int128 *)(r), __ATOMIC_SEQ_CST); 1; })
#        define ins_atomic_u128_eval_cond_assign(x, k, c) \
            (b32) __atomic_compare_exchange((__int128 *)(x), (__int128 *)(c), (__int128 *)&(k), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#    elif ARCH_ARM64
#        define ins_atomic_u128_eval(x, r) \
            ({ __atomic_thread_fence(__ATOMIC_SEQ_CST); \
*(u128 *)(r) = *(volatile u128 *)(x); \
__atomic_thread_fence(__ATOMIC_SEQ_CST); 1; })
#        define ins_atomic_u128_eval_cond_assign(x, k, c) \
            (b32) __atomic_compare_exchange_n((__uint128_t *)(x), (__uint128_t *)(c), *(__uint128_t *)&(k), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#    else
#        error "128-bit atomics not defined for this architecture"
#    endif
#    define ins_atomic_u64_eval(x)                   __atomic_load_n(x, __ATOMIC_SEQ_CST)
#    define ins_atomic_u64_inc_eval(x)               (__atomic_fetch_add((u64 *)(x), 1, __ATOMIC_SEQ_CST) + 1)
#    define ins_atomic_u64_dec_eval(x)               (__atomic_fetch_sub((u64 *)(x), 1, __ATOMIC_SEQ_CST) - 1)
#    define ins_atomic_u64_eval_assign(x, c)         __atomic_exchange_n(x, c, __ATOMIC_SEQ_CST)
#    define ins_atomic_u64_add_eval(x, c)            (__atomic_fetch_add((u64 *)(x), c, __ATOMIC_SEQ_CST) + (c))
#    define ins_atomic_u64_eval_cond_assign(x, k, c) ({ u64 _new = (c); __atomic_compare_exchange_n((u64 *)(x),&_new,(k),0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST); _new; })
#    define ins_atomic_u32_eval(x)                   __atomic_load_n(x, __ATOMIC_SEQ_CST)
#    define ins_atomic_u32_inc_eval(x)               (__atomic_fetch_add((u32 *)(x), 1, __ATOMIC_SEQ_CST) + 1)
#    define ins_atomic_u32_dec_eval(x)               (__atomic_fetch_sub((u32 *)(x), 1, __ATOMIC_SEQ_CST) - 1)
#    define ins_atomic_u32_add_eval(x, c)            (__atomic_fetch_add((u32 *)(x), c, __ATOMIC_SEQ_CST) + (c))
#    define ins_atomic_u32_eval_assign(x, c)         __atomic_exchange_n((x), (c), __ATOMIC_SEQ_CST)
#    define ins_atomic_u32_eval_cond_assign(x, k, c) ({ u32 _new = (c); __atomic_compare_exchange_n((u32 *)(x),&_new,(k),0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST); _new; })
#    define ins_atomic_u8_eval_assign(x, c)          __atomic_exchange_n((x), (c), __ATOMIC_SEQ_CST)
#else
#    error Atomic intrinsics not defined for this compiler / architecture.
#endif

#if ARCH_ADDRSIZE == 64
#    define ins_atomic_ptr_eval_cond_assign(x, k, c) (void *)ins_atomic_u64_eval_cond_assign((u64 *)(x), (u64)(k), (u64)(c))
#    define ins_atomic_ptr_eval_assign(x, c)         (void *)ins_atomic_u64_eval_assign((u64 *)(x), (u64)(c))
#    define ins_atomic_ptr_eval(x)                   (void *)ins_atomic_u64_eval((u64 *)x)
#elif ARCH_ADDRSIZE == 32
#    define ins_atomic_ptr_eval_cond_assign(x, k, c) (void *)ins_atomic_u32_eval_cond_assign((u32 *)(x), (u32)(k), (u32)(c))
#    define ins_atomic_ptr_eval_assign(x, c)         (void *)ins_atomic_u32_eval_assign((u32 *)(x), (u32)(c))
#    define ins_atomic_ptr_eval(x)                   (void *)ins_atomic_u32_eval((u32 *)x)
#else
#    error Atomic intrinsics for pointers not defined for this architecture.
#endif

#if ARCH_ADDRSIZE == 32
typedef u32 uaddr;
typedef s32 saddr;
#elif ARCH_ADDRSIZE == 64
typedef u64 uaddr;
typedef s64 saddr;
#else
#    error uaddr and saddr not defined for this architecture
#endif

typedef void VoidFunc(void);

typedef enum Axis2 {
    Axis2_X = 0,
    Axis2_Y,
    Axis2_COUNT
} Axis2;

typedef enum Axis {
    Axis_X = 0,
    Axis_Y = 1,
    Axis_Z = 2,
    Axis_W = 3,
    Axis_COUNT
} Axis;

typedef enum Corner {
    Corner_Invalid = -1,
    Corner_00,
    Corner_01,
    Corner_10,
    Corner_11,
    Corner_COUNT
} Corner;

typedef enum Side {
    Side_Min = 0,
    Side_Max = 1,
} Side;

typedef enum Operation_System {
    Operating_System_Null = 0,
    Operating_System_Windows = 1,
    Operating_System_Linux = 2,
    Operating_System_Mac = 3,
    Operation_System_COUNT,
#if OS_WINDOWS
    Operating_System_Current = Operating_System_Windows,
#elif OS_LINUX
    Operating_System_Current = Operating_System_Linux,
#elif OS_MAC
    Operating_System_Current = Operating_System_Mac,
#else
    Operating_System_Current = Operating_System_Null,
#endif
} Operating_System;

typedef enum Arch {
    Arch_Null = 0,
    Arch_x64 = 1,
    Arch_x86 = 2,
    Arch_Arm = 3,
    Arch_Arm64 = 4,
    Arch_COUNT,
} Arch;

typedef enum Month {
    Month_Jan = 1,
    Month_Feb = 2,
    Month_Mar = 3,
    Month_Apr = 4,
    Month_May = 5,
    Month_Jun = 6,
    Month_Jul = 7,
    Month_Aug = 8,
    Month_Sep = 9,
    Month_Oct = 10,
    Month_Nov = 11,
    Month_Dec = 12,
} Month;

typedef enum Day_Of_Week {
    Day_Of_Week_Sunday = 0,
    Day_Of_Week_Monday = 1,
    Day_Of_Week_Tuesday = 2,
    Day_Of_Week_Wednesday = 3,
    Day_Of_Week_Thursday = 4,
    Day_Of_Week_Friday = 5,
    Day_Of_Week_Saturday = 6
} DayOfWeek;

typedef enum Run_Mode {
    Run_Mode_Debug = 0,
    Run_Mode_Profile = 1,
    Run_Mode_Optimized = 2,
    Run_Mode_Debug_Profile = 3,
    Run_Mode_COUNT
} Run_Mode;

typedef u64 Dense_Time;

typedef struct Date_Time {
    u16 msec; // [0,999]
    u8  sec;  // [0,60]
    u8  min;  // [0,59]
    u8  hour; // [0,23]
    u8  day;  // [1,31]
    u8  mon;  // [1,12]
    s16 year; // 1 = 1 ce; 2020 = 2020 ce; 0 = 1 bce; -100 = 101 bce; etc.
} Date_Time;

static Dense_Time dense_time_from_date_time(Date_Time date_time);
static Date_Time  date_time_from_dense_time(Dense_Time time);
