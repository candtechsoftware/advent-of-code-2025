#pragma once
typedef struct Lane_Ctx Lane_Ctx;
struct Lane_Ctx {
    u64     lane_idx;         // This thread's lane index (0-based)
    u64     lane_count;       // Total number of lanes
    Barrier barrier;          // Sync point for lane_sync()
    void   *broadcast_memory; // Shared buffer for broadcasting (64 bytes)
};

typedef struct Rng1U64 Rng1U64;
struct Rng1U64 {
    u64 min;
    u64 max; // exclusive
};

typedef struct TCTX TCTX;
struct TCTX {
    Arena   *arenas[2];
    u8       thread_name[32];
    u64      thread_name_size;
    Lane_Ctx lane_ctx;
};

TCTX               *tctx_alloc(void);
void                tctx_release(TCTX *tctx);
static inline void  tctx_select(TCTX *tctx);
static inline TCTX *tctx_selected(void);

Arena *tctx_get_scratch(Arena **conflicts, u64 count);

#define scratch_begin(conflicts, count) arena_begin_scratch(tctx_get_scratch((conflicts), (count)))
#define scratch_end(scratch)            arena_end_scratch(&(scratch))

void   tctx_set_thread_name(String name);
String tctx_get_thread_name(void);

void    tctx_lane_init(u64 lane_idx, u64 lane_count, Barrier barrier, void *broadcast_mem);
void    tctx_lane_sync(void *broadcast_ptr, u64 broadcast_size, u64 broadcast_src_lane_idx);
Rng1U64 tctx_lane_range(u64 total_count);

#define lane_idx()                   (tctx_selected()->lane_ctx.lane_idx)
#define lane_count()                 (tctx_selected()->lane_ctx.lane_count)
#define lane_from_task_idx(idx)      ((idx) % lane_count())
#define lane_sync()                  tctx_lane_sync(0, 0, 0)
#define lane_sync_u64(ptr, src_lane) tctx_lane_sync((ptr), sizeof(u64), (src_lane))
#define lane_range(count)            tctx_lane_range(count)
