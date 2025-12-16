#pragma once

typedef u64 (*Sort_Key_Func)(void *element);

void radix_sort_u64(void *elements, u64 count, u64 element_size, Sort_Key_Func get_key, Arena *arena);

void radix_sort_u64_lane(void *elements, u64 count, u64 element_size, Sort_Key_Func get_key,
                         void *temp_buffer, u64 *lane_histograms, u64 *global_histogram);
