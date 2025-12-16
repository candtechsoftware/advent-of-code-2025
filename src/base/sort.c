#include "sort.h"

#define RADIX_BITS     8
#define RADIX_BUCKETS  (1 << RADIX_BITS)
#define RADIX_MASK     (RADIX_BUCKETS - 1)
#define RADIX_PASSES   (64 / RADIX_BITS)

void
radix_sort_u64(void *elements, u64 count, u64 element_size, Sort_Key_Func get_key, Arena *arena) {
    if (count <= 1) return;

    u8 *src = (u8 *)elements;
    u8 *dst = push_array(arena, u8, count * element_size);
    u64 histogram[RADIX_BUCKETS];

    for (u32 pass = 0; pass < RADIX_PASSES; pass++) {
        u32 shift = pass * RADIX_BITS;

        for (u64 i = 0; i < RADIX_BUCKETS; i++) {
            histogram[i] = 0;
        }

        for (u64 i = 0; i < count; i++) {
            u64 key = get_key(src + i * element_size);
            u64 bucket = (key >> shift) & RADIX_MASK;
            histogram[bucket]++;
        }

        u64 offset = 0;
        for (u64 i = 0; i < RADIX_BUCKETS; i++) {
            u64 c = histogram[i];
            histogram[i] = offset;
            offset += c;
        }

        for (u64 i = 0; i < count; i++) {
            void *elem = src + i * element_size;
            u64 key = get_key(elem);
            u64 bucket = (key >> shift) & RADIX_MASK;
            u64 dst_idx = histogram[bucket]++;
            MemoryCopy(dst + dst_idx * element_size, elem, element_size);
        }

        u8 *tmp = src;
        src = dst;
        dst = tmp;
    }

    if (src != (u8 *)elements) {
        MemoryCopy(elements, src, count * element_size);
    }
}

void
radix_sort_u64_lane(void *elements, u64 count, u64 element_size, Sort_Key_Func get_key,
                    void *temp_buffer, u64 *lane_histograms, u64 *global_histogram) {
    if (count <= 1) return;

    u64 my_lane = lane_idx();
    u64 num_lanes = lane_count();
    u64 *my_histogram = lane_histograms + my_lane * RADIX_BUCKETS;

    u8 *src = (u8 *)elements;
    u8 *dst = (u8 *)temp_buffer;

    Rng1U64 my_range = lane_range(count);

    for (u32 pass = 0; pass < RADIX_PASSES; pass++) {
        u32 shift = pass * RADIX_BITS;

        for (u64 i = 0; i < RADIX_BUCKETS; i++) {
            my_histogram[i] = 0;
        }

        for (u64 i = my_range.min; i < my_range.max; i++) {
            u64 key = get_key(src + i * element_size);
            u64 bucket = (key >> shift) & RADIX_MASK;
            my_histogram[bucket]++;
        }

        lane_sync();

        if (my_lane == 0) {
            for (u64 b = 0; b < RADIX_BUCKETS; b++) {
                u64 total = 0;
                for (u64 lane = 0; lane < num_lanes; lane++) {
                    u64 c = lane_histograms[lane * RADIX_BUCKETS + b];
                    lane_histograms[lane * RADIX_BUCKETS + b] = total;
                    total += c;
                }
                global_histogram[b] = total;
            }

            u64 offset = 0;
            for (u64 b = 0; b < RADIX_BUCKETS; b++) {
                u64 c = global_histogram[b];
                global_histogram[b] = offset;
                offset += c;
            }
        }

        lane_sync();

        for (u64 i = my_range.min; i < my_range.max; i++) {
            void *elem = src + i * element_size;
            u64 key = get_key(elem);
            u64 bucket = (key >> shift) & RADIX_MASK;
            u64 global_offset = global_histogram[bucket];
            u64 lane_offset = my_histogram[bucket]++;
            u64 dst_idx = global_offset + lane_offset;
            MemoryCopy(dst + dst_idx * element_size, elem, element_size);
        }

        lane_sync();

        u8 *tmp = src;
        src = dst;
        dst = tmp;
    }

    lane_sync();
    if (my_lane == 0 && src != (u8 *)elements) {
        MemoryCopy(elements, src, count * element_size);
    }
    lane_sync();
}
