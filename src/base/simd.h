#pragma once
#include "base.h"

// SIMD Abstraction Layer
// Portable SIMD operations for parsing, rendering, and general computation.
// Supports: ARM NEON, x86 SSE4/AVX2, and scalar fallback.
// Naming: simd_{operation}_{type}

typedef struct Simd_V16u8 Simd_V16u8;
typedef struct Simd_V4f32 Simd_V4f32;
typedef struct Simd_V4s32 Simd_V4s32;
typedef struct Simd_V4u32 Simd_V4u32;

#if USE_NEON
struct Simd_V16u8 { uint8x16_t v; };
struct Simd_V4f32 { float32x4_t v; };
struct Simd_V4s32 { int32x4_t v; };
struct Simd_V4u32 { uint32x4_t v; };
#elif USE_SSE4 || USE_AVX2
struct Simd_V16u8 { __m128i v; };
struct Simd_V4f32 { __m128 v; };
struct Simd_V4s32 { __m128i v; };
struct Simd_V4u32 { __m128i v; };
#else
struct Simd_V16u8 { u8 v[16]; };
struct Simd_V4f32 { f32 v[4]; };
struct Simd_V4s32 { s32 v[4]; };
struct Simd_V4u32 { u32 v[4]; };
#endif

// ============================================================================
// SIMD Function Reference
// ============================================================================
//
// LOAD/STORE
// simd_loadu_u8(ptr)         - Load 16 bytes from unaligned memory
//                              @example ptr=[1,2,3,...,16] -> {1,2,3,...,16}
// simd_loadu_f32(ptr)        - Load 4 floats from unaligned memory
//                              @example ptr=[1.0,2.0,3.0,4.0] -> {1.0,2.0,3.0,4.0}
// simd_loadu_s32(ptr)        - Load 4 s32 from unaligned memory
//                              @example ptr=[1,2,3,4] -> {1,2,3,4}
// simd_loadu_u32(ptr)        - Load 4 u32 from unaligned memory
//                              @example ptr=[1,2,3,4] -> {1,2,3,4}
// simd_storeu_u8(ptr, v)     - Store 16 bytes to unaligned memory
// simd_storeu_f32(ptr, v)    - Store 4 floats to unaligned memory
// simd_storeu_s32(ptr, v)    - Store 4 s32 to unaligned memory
// simd_storeu_u32(ptr, v)    - Store 4 u32 to unaligned memory
//
// SET/BROADCAST
// simd_set1_u8(val)          - Broadcast byte to all 16 lanes
//                              @example val=42 -> {42,42,42,...,42}
// simd_set1_f32(val)         - Broadcast float to all 4 lanes
//                              @example val=3.14 -> {3.14,3.14,3.14,3.14}
// simd_set1_s32(val)         - Broadcast s32 to all 4 lanes
//                              @example val=42 -> {42,42,42,42}
// simd_set1_u32(val)         - Broadcast u32 to all 4 lanes
// simd_set_f32(a,b,c,d)      - Set 4 floats individually
//                              @example (1.0,2.0,3.0,4.0) -> {1.0,2.0,3.0,4.0}
// simd_set_s32(a,b,c,d)      - Set 4 s32 individually
// simd_set_u32(a,b,c,d)      - Set 4 u32 individually
// simd_zero_u8()             - Create zero vector (16 bytes)
// simd_zero_f32()            - Create zero vector (4 floats)
// simd_zero_s32()            - Create zero vector (4 s32)
// simd_zero_u32()            - Create zero vector (4 u32)
//
// ARITHMETIC
// simd_add_f32(a, b)         - Add floats element-wise
//                              @example {1,2,3,4}+{10,20,30,40} -> {11,22,33,44}
// simd_sub_f32(a, b)         - Subtract floats element-wise
// simd_mul_f32(a, b)         - Multiply floats element-wise
// simd_div_f32(a, b)         - Divide floats element-wise
// simd_add_s32(a, b)         - Add s32 element-wise
// simd_sub_s32(a, b)         - Subtract s32 element-wise
// simd_mul_s32(a, b)         - Multiply s32 element-wise
// simd_fmadd_f32(a, b, c)    - Fused multiply-add: a*b+c
//                              @example {1,2,3,4}*{2,2,2,2}+{10,10,10,10} -> {12,14,16,18}
// simd_neg_f32(a)            - Negate floats
//                              @example {1,-2,3,-4} -> {-1,2,-3,4}
// simd_neg_s32(a)            - Negate s32
// simd_abs_f32(a)            - Absolute value of floats
//                              @example {-1,2,-3,4} -> {1,2,3,4}
// simd_abs_s32(a)            - Absolute value of s32
// simd_sqrt_f32(a)           - Square root of floats
//                              @example {1,4,9,16} -> {1,2,3,4}
//
// MIN/MAX
// simd_min_u8(a, b)          - Element-wise minimum of bytes
// simd_max_u8(a, b)          - Element-wise maximum of bytes
// simd_min_f32(a, b)         - Element-wise minimum of floats
// simd_max_f32(a, b)         - Element-wise maximum of floats
// simd_min_s32(a, b)         - Element-wise minimum of s32
// simd_max_s32(a, b)         - Element-wise maximum of s32
//
// COMPARE (returns mask: all 1s for true, all 0s for false)
// simd_cmpeq_u8(a, b)        - Compare bytes for equality
//                              @example {1,2,3,4},{1,9,3,9} -> {FF,00,FF,00,...}
// simd_cmpgt_u8(a, b)        - Compare bytes for greater-than (unsigned)
// simd_cmpeq_f32(a, b)       - Compare floats for equality
// simd_cmpgt_f32(a, b)       - Compare floats for greater-than
// simd_cmplt_f32(a, b)       - Compare floats for less-than
// simd_cmpge_f32(a, b)       - Compare floats for greater-or-equal
// simd_cmple_f32(a, b)       - Compare floats for less-or-equal
// simd_cmpeq_s32(a, b)       - Compare s32 for equality
// simd_cmpgt_s32(a, b)       - Compare s32 for greater-than
//
// LOGICAL/BITWISE
// simd_and_u8(a, b)          - Bitwise AND of bytes
// simd_or_u8(a, b)           - Bitwise OR of bytes
// simd_xor_u8(a, b)          - Bitwise XOR of bytes
// simd_not_u8(a)             - Bitwise NOT of bytes
// simd_andnot_u8(a, b)       - Bitwise AND-NOT: (~a) & b
// simd_and_s32(a, b)         - Bitwise AND of s32
// simd_or_s32(a, b)          - Bitwise OR of s32
// simd_xor_s32(a, b)         - Bitwise XOR of s32
//
// MASK OPERATIONS
// simd_movemask_u8(a)        - Extract high bit of each byte into 16-bit mask
//                              @example {0x80,0x00,0x80,0x00,...} -> 0b0101...
// simd_movemask_f32(a)       - Extract sign bit of each float into 4-bit mask
//                              @example {-1.0,1.0,-1.0,1.0} -> 0b0101
// simd_movemask_s32(a)       - Extract sign bit of each s32 into 4-bit mask
// simd_blend_u8(a, b, mask)  - Blend bytes: mask ? b : a (high bit selects)
//                              @example blend({1,2},{10,20},{0,FF}) -> {1,20}
// simd_blend_f32(a, b, mask) - Blend floats: sign bit selects
// simd_blend_s32(a, b, mask) - Blend s32: sign bit selects
//
// HORIZONTAL OPERATIONS
// simd_hsum_s32(a)           - Horizontal sum of s32
//                              @example {1,2,3,4} -> 10
// simd_hsum_f32(a)           - Horizontal sum of floats
// simd_hmin_s32(a)           - Horizontal minimum of s32
//                              @example {5,2,8,1} -> 1
// simd_hmax_s32(a)           - Horizontal maximum of s32
// simd_hmin_f32(a)           - Horizontal minimum of floats
// simd_hmax_f32(a)           - Horizontal maximum of floats
//
// CONVERSION
// simd_cvt_s32_f32(a)        - Convert s32 to f32
//                              @example {1,2,3,4} -> {1.0,2.0,3.0,4.0}
// simd_cvt_f32_s32(a)        - Convert f32 to s32 (truncate toward zero)
//                              @example {1.9,2.1,3.5,4.9} -> {1,2,3,4}
//
// UTILITY
// simd_ctz(mask)             - Count trailing zeros. Undefined if mask==0
//                              @example 0b1000 -> 3
// simd_clz(mask)             - Count leading zeros. Undefined if mask==0
// simd_popcount(mask)        - Population count (number of set bits)
//                              @example 0b10101010 -> 4
// simd_any_true_u8(a)        - Check if any byte is non-zero
// simd_all_true_u8(a)        - Check if all bytes are 0xFF
//
// SHUFFLE
// simd_shuffle_u8(a, idx)    - Shuffle bytes using index vector. Index 0x80+ -> 0
//                              @example shuffle({a,b,c,d},{3,2,1,0}) -> {d,c,b,a}
