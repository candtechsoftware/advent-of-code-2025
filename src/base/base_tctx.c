thread_static TCTX *tctx_thread_local = 0;

TCTX *
tctx_alloc(void) {
    Arena *arena = arena_alloc();
    TCTX  *tctx = push_array(arena, TCTX, 1);
    tctx->arenas[0] = arena;
    tctx->arenas[1] = arena_alloc();
    return tctx;
}

void tctx_release(TCTX *tctx) {
    arena_release(tctx->arenas[1]);
    arena_release(tctx->arenas[0]);
}

static inline void tctx_select(TCTX *tctx) {
    tctx_thread_local = tctx;
}

static inline TCTX *
tctx_selected(void) {
    return tctx_thread_local;
}

Arena *
tctx_get_scratch(Arena **conflicts, u64 count) {
    TCTX *tctx = tctx_selected();
    if (!tctx) {
        return arena_alloc();
    }
    Arena  *result = 0;
    Arena **arena_ptr = tctx->arenas;
    for (u64 i = 0; i < ArrayCount(tctx->arenas); i += 1, arena_ptr += 1) {
        Arena **conflict_ptr = conflicts;
        b32     has_conflict = 0;
        for (u64 j = 0; j < count; j += 1, conflict_ptr += 1) {
            if (*arena_ptr == *conflict_ptr) {
                has_conflict = 1;
                break;
            }
        }
        if (!has_conflict) {
            result = *arena_ptr;
            break;
        }
    }
    return result;
}

void tctx_set_thread_name(String string) {
    TCTX *tctx = tctx_selected();
    u64   size = ClampTop(string.size, sizeof(tctx->thread_name));
    MemoryCopy(tctx->thread_name, string.str, size);
    tctx->thread_name_size = size;
    os_set_thread_name(string);
}

String
tctx_get_thread_name(void) {
    TCTX  *tctx = tctx_selected();
    String result = {tctx->thread_name, tctx->thread_name_size};
    return result;
}

void
tctx_lane_init(u64 lane_idx, u64 lane_count, Barrier barrier, void *broadcast_mem) {
    TCTX *tctx = tctx_selected();
    tctx->lane_ctx.lane_idx = lane_idx;
    tctx->lane_ctx.lane_count = lane_count;
    tctx->lane_ctx.barrier = barrier;
    tctx->lane_ctx.broadcast_memory = broadcast_mem;
}

void
tctx_lane_sync(void *broadcast_ptr, u64 broadcast_size, u64 broadcast_src_lane_idx) {
    TCTX *tctx = tctx_selected();
    Lane_Ctx *lane = &tctx->lane_ctx;
	
    // Skip if no barrier configured
    if (lane->barrier.v[0] == 0) {
        return;
    }
	
    u64 broadcast_size_clamped = ClampTop(broadcast_size, 64);
	
    if (broadcast_ptr && lane->lane_idx == broadcast_src_lane_idx && lane->broadcast_memory) {
        MemoryCopy(lane->broadcast_memory, broadcast_ptr, broadcast_size_clamped);
    }
	
    barrier_wait(lane->barrier);
	
    if (broadcast_ptr && lane->lane_idx != broadcast_src_lane_idx && lane->broadcast_memory) {
        MemoryCopy(broadcast_ptr, lane->broadcast_memory, broadcast_size_clamped);
    }
	
    if (broadcast_ptr) {
        barrier_wait(lane->barrier);
    }
}

Rng1U64
tctx_lane_range(u64 total_count) {
    TCTX *tctx = tctx_selected();
    Lane_Ctx *lane = &tctx->lane_ctx;
	
    Rng1U64 result = {0};
	
    if (lane->lane_count == 0 || total_count == 0) {
        return result;
    }
	
    u64 per_lane = total_count / lane->lane_count;
    u64 remainder = total_count % lane->lane_count;
	
    if (lane->lane_idx < remainder) {
        result.min = lane->lane_idx * (per_lane + 1);
        result.max = result.min + per_lane + 1;
    } else {
        result.min = remainder * (per_lane + 1) + (lane->lane_idx - remainder) * per_lane;
        result.max = result.min + per_lane;
    }
	
    return result;
}
