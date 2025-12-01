#pragma once

#ifndef ARENA_FREE_LIST
#    define ARENA_FREE_LIST 1
#endif

#ifndef ARENA_DEBUG
#    define ARENA_DEBUG 0
#endif

void *os_reserve(u64 size);
void *os_reserve_large(u64 size);
b32   os_commit(void *ptr, u64 size);
void  os_decommit(void *ptr, u64 size);
void  os_mem_release(void *ptr, u64 size);

typedef enum ArenaFlags {
    ArenaFlag_NoChain = (1 << 0),    // Prevent automatic chunk chaining (fail if out of space)
    ArenaFlag_LargePages = (1 << 1), // Use OS large pages (2MB) for better TLB performance
} ArenaFlags;

typedef struct Arena Arena;
struct Arena {
    Arena     *current;
    Arena     *prev;
    ArenaFlags flags;
    u64        default_reserve_size;
    u64        default_commit_size;
    u16        alignment;
    b8         growing;
    u8         filler;
    u64        base_pos;
    u64        chunk_pos;
    u64        chunk_cap;
    u64        chunk_commit_pos;
#if ARENA_FREE_LIST
    Arena *free_last;
#endif
#if ARENA_DEBUG
    char *allocation_site_file;
    int   allocation_site_line;
#endif
};

#define MEM_COMMIT_BLOCK_SIZE   MB(64)
#define MEM_MAX_ALIGN           64
#define MEM_VSCRATCH_POOL_COUNT 2
#define MEM_INITIAL_COMMIT      KB(4)
#define MEM_INTERNAL_MIN_SIZE   AlignUpPow2(sizeof(Arena), MEM_MAX_ALIGN)

StaticAssert(sizeof(Arena) <= MEM_INITIAL_COMMIT, mem_check_arena_size);

StaticAssert(IsPow2OrZero(MEM_COMMIT_BLOCK_SIZE) &&
                 MEM_COMMIT_BLOCK_SIZE != 0,
             mem_check_commit_block_size);

StaticAssert(IsPow2OrZero(MEM_MAX_ALIGN) && MEM_MAX_ALIGN != 0,
             mem_check_max_align);

typedef struct Scratch Scratch;
struct Scratch {
    Arena *arena;
    u64    pos;
};

typedef struct Arena_Params Arena_Params;
struct Arena_Params {
    ArenaFlags flags;
    u64        reserve_size;
    u64        commit_size;
    u64        alignment;
    void      *backing_buffer;
#if ARENA_DEBUG
    char *allocation_site_file;
    int   allocation_site_line;
#endif
};

static Arena     *arena_new(u64 reserve_size, u64 alignment, b32 growing);
static Arena     *arena_alloc_(Arena_Params *params);
static void       arena_release(Arena *arena);
static void      *arena_push_no_zero(Arena *arena, u64 size);
static void      *arena_push(Arena *arena, u64 size);
static void       arena_pop_to(Arena *arena, u64 pos);
static inline u64 arena_current_pos(Arena *arena);
#if ARENA_FREE_LIST
static void arena_trim(Arena *arena);
#endif

static inline Scratch arena_get_scratch(Arena **conflic_array, u32 count);
static inline Scratch arena_begin_scratch(Arena *arena);
static inline void    arena_end_scratch(Scratch *sc);

#if ARENA_DEBUG
void arena_debug_print(Arena *arena);
#endif

#if ARENA_DEBUG
#    define arena_alloc(...) arena_alloc_(&(Arena_Params){ \
        .reserve_size = GB(1),                             \
        .commit_size = MEM_COMMIT_BLOCK_SIZE,              \
        .alignment = 8,                                    \
        .allocation_site_file = __FILE__,                  \
        .allocation_site_line = __LINE__,                  \
        __VA_ARGS__})
#else
#    define arena_alloc(...) arena_alloc_(&(Arena_Params){ \
        .reserve_size = GB(1),                             \
        .commit_size = MEM_COMMIT_BLOCK_SIZE,              \
        .alignment = 8,                                    \
        __VA_ARGS__})
#endif

#define push_array(a, T, c)         (T *)arena_push((a), sizeof(T) * (c))
#define push_array_no_zero(a, T, c) (T *)arena_push_no_zero((a), sizeof(T) * (c))
#define push_array_copy(a, T, c, p) (T *)(MemoryCopy(push_array(a, T, c), (p), sizeof(T) * (c)))
#define arena_reset(a)              arena_pop_to(a, 0)
