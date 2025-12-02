#pragma once
#include "base.h"

typedef struct simd_v4s32 simd_v4s32;
typedef struct simd_v8s32 simd_v8s32;

#if USE_NEON
struct simd_v4s32 {
    int32x4_t v;
};
#elif USE_AVX2
struct simd_v8s32 {
    __m256i v;
};
#elif USE_SSE4
struct simd_v4s32 {
    __m128i v;
};
#else
struct simd_v4s32 {
    s32 v[4];
};
#endif

simd_v4s32 simd_load4_s32(s32 *ptr);
void simd_store4_s32(s32 *ptr, simd_v4s32 val);
simd_v4s32 simd_set1_s32(s32 val);
simd_v4s32 simd_add_s32(simd_v4s32 a, simd_v4s32 b);
simd_v4s32 simd_sub_s32(simd_v4s32 a, simd_v4s32 b);
simd_v4s32 simd_neg_s32(simd_v4s32 a);
simd_v4s32 simd_blend_s32(simd_v4s32 a, simd_v4s32 b, simd_v4s32 mask);
u32 simd_cmpeq_zero_mask_s32(simd_v4s32 a);
simd_v4s32 simd_mul_s32(simd_v4s32 a, simd_v4s32 b);
s32 simd_hsum_s32(simd_v4s32 a);
simd_v4s32 simd_set_s32(s32 a, s32 b, s32 c, s32 d);
