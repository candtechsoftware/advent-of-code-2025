static Arena *
arena_alloc_(Arena_Params *params) {
    ProfBeginFunc();
    Arena *res = 0;

    u64        reserve_size = params->reserve_size ? params->reserve_size : GB(1);
    u64        commit_size = params->commit_size ? params->commit_size : MEM_COMMIT_BLOCK_SIZE;
    u64        alignment = params->alignment ? params->alignment : 8;
    ArenaFlags flags = params->flags;
    b32        growing = !(flags & ArenaFlag_NoChain);

    if (reserve_size >= MEM_INITIAL_COMMIT) {
        void *mem = 0;

        if (params->backing_buffer) {
            mem = params->backing_buffer;
        } else if (flags & ArenaFlag_LargePages) {
            mem = os_reserve_large(reserve_size);
        } else {
            mem = os_reserve(reserve_size);
        }

        if (mem && (params->backing_buffer || os_commit(mem, MEM_INITIAL_COMMIT))) {
            res = (Arena *)mem;
            res->current = res;
            res->prev = 0;
            res->flags = flags;
            res->alignment = (u16)alignment;
            res->default_reserve_size = reserve_size;
            res->default_commit_size = commit_size;
            res->growing = (b8)growing;
            res->base_pos = 0;
            res->chunk_cap = reserve_size;
            res->chunk_pos = MEM_INTERNAL_MIN_SIZE;
            res->chunk_commit_pos = params->backing_buffer ? reserve_size : MEM_INITIAL_COMMIT;

#if ARENA_FREE_LIST
            res->free_last = 0;
#endif

#if ARENA_DEBUG
            res->allocation_site_file = params->allocation_site_file;
            res->allocation_site_line = params->allocation_site_line;
#endif

            if (!params->backing_buffer && res->chunk_commit_pos < res->chunk_cap) {
                void *poison_start = (u8 *)res + res->chunk_commit_pos;
                u64   poison_size = res->chunk_cap - res->chunk_commit_pos;
                AsanPoisonMemoryRegion(poison_start, poison_size);
            }

            if (res->chunk_pos < res->chunk_commit_pos) {
                void *poison_start = (u8 *)res + res->chunk_pos;
                u64   poison_size = res->chunk_commit_pos - res->chunk_pos;
                AsanPoisonMemoryRegion(poison_start, poison_size);
            }
        }
    }
    Assert(res != 0);
    ProfEndFunc();
    return res;
}

static Arena *
arena_new(u64 reserve_size, u64 alignment, b32 growing) {
    Arena_Params params = {0};
    params.reserve_size = reserve_size;
    params.alignment = alignment;
    if (!growing) {
        params.flags = ArenaFlag_NoChain;
    }
    return arena_alloc_(&params);
}

static void
arena_release(Arena *arena) {
    if (!arena)
        return;

#if ARENA_FREE_LIST
    Arena *free_chunk = arena->free_last;
    while (free_chunk) {
        Arena *next = free_chunk->prev;
        AsanUnpoisonMemoryRegion(free_chunk, free_chunk->chunk_cap);
        os_mem_release(free_chunk, free_chunk->chunk_cap);
        free_chunk = next;
    }
#endif
    Arena *current = arena->current;
    while (current) {
        Arena *prev = current->prev;
        AsanUnpoisonMemoryRegion(current, current->chunk_cap);
        os_mem_release(current, current->chunk_cap);
        current = prev;
    }
}

static void *
arena_push(Arena *arena, u64 size) {
    if (!arena)
        return 0;

    Arena *current = arena->current;
    u64    pos_mem = current->chunk_pos;
    u64    align = arena->alignment;
    u64    pos_mem_aligned = AlignUpPow2(pos_mem, align);
    u64    new_pos = pos_mem_aligned + size;

    if (new_pos > current->chunk_cap && !arena->growing) {
        return 0;
    }

    if (new_pos > current->chunk_cap && arena->growing) {
        u64 new_chunk_size = arena->default_reserve_size;
        if (new_chunk_size < size + MEM_INTERNAL_MIN_SIZE) {
            new_chunk_size = AlignUpPow2(size + MEM_INTERNAL_MIN_SIZE, KB(4));
        }

        Arena *new_chunk = 0;

#if ARENA_FREE_LIST
        Arena **free_ptr = &arena->free_last;
        while (*free_ptr) {
            Arena *candidate = *free_ptr;
            if (candidate->chunk_cap >= new_chunk_size) {
                *free_ptr = candidate->prev;
                new_chunk = candidate;
                new_chunk->chunk_pos = MEM_INTERNAL_MIN_SIZE;
                if (new_chunk->chunk_pos < new_chunk->chunk_commit_pos) {
                    void *poison_start = (u8 *)new_chunk + new_chunk->chunk_pos;
                    u64   poison_size = new_chunk->chunk_commit_pos - new_chunk->chunk_pos;
                    AsanPoisonMemoryRegion(poison_start, poison_size);
                }
                break;
            }
            free_ptr = &candidate->prev;
        }
#endif

        if (!new_chunk) {
            Arena_Params chunk_params = {0};
            chunk_params.flags = arena->flags & ~ArenaFlag_NoChain;
            chunk_params.reserve_size = new_chunk_size;
            chunk_params.commit_size = arena->default_commit_size;
            chunk_params.alignment = align;

            new_chunk = arena_alloc_(&chunk_params);
            if (!new_chunk)
                return 0;
        }

        new_chunk->prev = current;
        new_chunk->base_pos = current->base_pos + current->chunk_cap;
        arena->current = new_chunk;
        current = new_chunk;

        pos_mem = current->chunk_pos;
        pos_mem_aligned = AlignUpPow2(pos_mem, align);
        new_pos = pos_mem_aligned + size;
    }

    if (new_pos > current->chunk_commit_pos) {
        u64 commit_size_aligned = AlignUpPow2(new_pos, MEM_COMMIT_BLOCK_SIZE);
        u64 new_commit_pos = Min(commit_size_aligned, current->chunk_cap);
        u64 commit_size = new_commit_pos - current->chunk_commit_pos;

        void *commit_ptr = (u8 *)current + current->chunk_commit_pos;
        b32   commit_result = os_commit(commit_ptr, commit_size);
        if (!commit_result) {
            log_error("[arena] Failed to commit %llu bytes at %p (errno=%d)\n", commit_size, commit_ptr, errno);
            return 0;
        }
        AsanUnpoisonMemoryRegion(commit_ptr, commit_size);
        current->chunk_commit_pos = new_commit_pos;
    }

    if (new_pos > current->chunk_commit_pos) {
        return 0;
    }

    void *result = (u8 *)current + pos_mem_aligned;
    u64   unpoison_size = new_pos - pos_mem;
    AsanUnpoisonMemoryRegion((u8 *)current + pos_mem, unpoison_size);
    current->chunk_pos = new_pos;

    MemoryZero(result, size);
    return result;
}

static void *
arena_push_no_zero(Arena *arena, u64 size) {
    if (!arena)
        return 0;

    Arena *current = arena->current;
    u64    pos_mem = current->chunk_pos;
    u64    align = arena->alignment;
    u64    pos_mem_aligned = AlignUpPow2(pos_mem, align);
    u64    new_pos = pos_mem_aligned + size;

    if (new_pos > current->chunk_cap)
        return 0;

    if (new_pos > current->chunk_commit_pos) {
        u64 commit_size = new_pos - current->chunk_commit_pos;
        commit_size = AlignUpPow2(commit_size, MEM_COMMIT_BLOCK_SIZE);
        u64 new_commit_pos = current->chunk_commit_pos + commit_size;
        if (new_commit_pos > current->chunk_cap) {
            new_commit_pos = current->chunk_cap;
            commit_size = new_commit_pos - current->chunk_commit_pos;
        }

        void *commit_ptr = (u8 *)current + current->chunk_commit_pos;
        if (!os_commit(commit_ptr, commit_size)) {
            return 0;
        }
        AsanUnpoisonMemoryRegion(commit_ptr, commit_size);
        current->chunk_commit_pos = new_commit_pos;
    }

    void *result = (u8 *)current + pos_mem_aligned;
    u64   unpoison_size = new_pos - pos_mem;
    AsanUnpoisonMemoryRegion((u8 *)current + pos_mem, unpoison_size);
    current->chunk_pos = new_pos;
    return result;
}

static void
arena_pop_to(Arena *arena, u64 pos) {
    if (!arena)
        return;

    Arena *current = arena->current;

    while (current->prev && pos < current->base_pos) {
        Arena *to_free = current;
        current = current->prev;

        AsanPoisonMemoryRegion((u8 *)to_free + MEM_INTERNAL_MIN_SIZE,
                               to_free->chunk_commit_pos - MEM_INTERNAL_MIN_SIZE);

#if ARENA_FREE_LIST
        to_free->prev = arena->free_last;
        arena->free_last = to_free;
#else
        AsanUnpoisonMemoryRegion(to_free, to_free->chunk_cap);
        os_mem_release(to_free, to_free->chunk_cap);
#endif
    }

    arena->current = current;

    u64 chunk_relative_pos = pos - current->base_pos;
    if (chunk_relative_pos < MEM_INTERNAL_MIN_SIZE) {
        chunk_relative_pos = MEM_INTERNAL_MIN_SIZE;
    }

    if (chunk_relative_pos < current->chunk_pos) {
        u64 freed_size = current->chunk_pos - chunk_relative_pos;
        AsanPoisonMemoryRegion((u8 *)current + chunk_relative_pos, freed_size);
        current->chunk_pos = chunk_relative_pos;
    }
}

static inline u64
arena_current_pos(Arena *arena) {
    if (!arena)
        return 0;
    Arena *current = arena->current;
    return current->base_pos + current->chunk_pos;
}

static inline Scratch
arena_get_scratch(Arena **conflic_array, u32 count) {
    Arena  *arena = tctx_get_scratch(conflic_array, (u64)count);
    Scratch s = {0};
    s.arena = arena;
    s.pos = arena_current_pos(arena);
    return s;
}

static inline Scratch
arena_begin_scratch(Arena *arena) {
    Scratch s = {0};
    if (arena) {
        s.arena = arena;
        s.pos = arena_current_pos(arena);
    } else {
        s = arena_get_scratch(0, 0);
    }
    return s;
}

static inline void
arena_end_scratch(Scratch *sc) {
    if (sc && sc->arena) {
        arena_pop_to(sc->arena, sc->pos);
    }
}

#if ARENA_FREE_LIST
void arena_trim(Arena *arena) {
    if (!arena)
        return;

    Arena *free_chunk = arena->free_last;
    while (free_chunk) {
        Arena *next = free_chunk->prev;
        AsanUnpoisonMemoryRegion(free_chunk, free_chunk->chunk_cap);
        os_mem_release(free_chunk, free_chunk->chunk_cap);
        free_chunk = next;
    }
    arena->free_last = 0;
}
#endif

#if ARENA_DEBUG
void arena_debug_print(Arena *arena) {
    if (!arena) {
        log_info("[arena_debug] Arena is NULL\n");
        return;
    }

    log_info("[arena_debug] Arena %p:\n", (void *)arena);
    log_info("  Allocation site: %s:%d\n",
             arena->allocation_site_file ? arena->allocation_site_file : "(unknown)",
             arena->allocation_site_line);
    log_info("  Flags: 0x%x (NoChain=%d, LargePages=%d)\n",
             arena->flags,
             (arena->flags & ArenaFlag_NoChain) ? 1 : 0,
             (arena->flags & ArenaFlag_LargePages) ? 1 : 0);
    log_info("  Growing: %d\n", arena->growing);
    log_info("  Alignment: %u\n", arena->alignment);
    log_info("  Default reserve: %llu bytes\n", arena->default_reserve_size);
    log_info("  Default commit: %llu bytes\n", arena->default_commit_size);

    u32 chunk_count = 0;
    u64 total_reserved = 0;
    u64 total_committed = 0;
    u64 total_used = 0;

    Arena *chunk = arena->current;
    while (chunk) {
        chunk_count++;
        total_reserved += chunk->chunk_cap;
        total_committed += chunk->chunk_commit_pos;
        total_used += chunk->chunk_pos;
        chunk = chunk->prev;
    }

    log_info("  Chunks: %u\n", chunk_count);
    log_info("  Total reserved: %llu bytes\n", total_reserved);
    log_info("  Total committed: %llu bytes\n", total_committed);
    log_info("  Total used: %llu bytes\n", total_used);

#    if ARENA_FREE_LIST
    u32    free_count = 0;
    Arena *free_chunk = arena->free_last;
    while (free_chunk) {
        free_count++;
        free_chunk = free_chunk->prev;
    }
    log_info("  Free list chunks: %u\n", free_count);
#    endif
}
#endif
