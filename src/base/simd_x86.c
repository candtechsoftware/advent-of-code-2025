#if USE_AVX2 || USE_SSE4

simd_v4s32
simd_load4_s32(s32 *ptr) {
#if USE_SSE4
    return (simd_v4s32){_mm_loadu_si128((__m128i*)ptr)};
#else
    return (simd_v4s32){{ptr[0], ptr[1], ptr[2], ptr[3]}};
#endif
}

void
simd_store4_s32(s32 *ptr, simd_v4s32 val) {
#if USE_SSE4
    _mm_storeu_si128((__m128i*)ptr, val.v);
#else
    ptr[0] = val.v[0];
    ptr[1] = val.v[1];
    ptr[2] = val.v[2];
    ptr[3] = val.v[3];
#endif
}

simd_v4s32
simd_set1_s32(s32 val) {
#if USE_SSE4
    return (simd_v4s32){_mm_set1_epi32(val)};
#else
    return (simd_v4s32){{val, val, val, val}};
#endif
}

simd_v4s32
simd_add_s32(simd_v4s32 a, simd_v4s32 b) {
#if USE_SSE4
    return (simd_v4s32){_mm_add_epi32(a.v, b.v)};
#else
    return (simd_v4s32){a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2], a.v[3] + b.v[3]};
#endif
}

simd_v4s32
simd_sub_s32(simd_v4s32 a, simd_v4s32 b) {
#if USE_SSE4
    return (simd_v4s32){_mm_sub_epi32(a.v, b.v)};
#else
    return (simd_v4s32){a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2], a.v[3] - b.v[3]};
#endif
}

simd_v4s32
simd_neg_s32(simd_v4s32 a) {
#if USE_SSE4
    return (simd_v4s32){_mm_sub_epi32(_mm_setzero_si128(), a.v)};
#else
    return (simd_v4s32){-a.v[0], -a.v[1], -a.v[2], -a.v[3]};
#endif
}

simd_v4s32
simd_blend_s32(simd_v4s32 a, simd_v4s32 b, simd_v4s32 mask) {
#if USE_SSE4
    return (simd_v4s32){_mm_blendv_epi8(a.v, b.v, mask.v)};
#else
    return (simd_v4s32){
        mask.v[0] ? b.v[0] : a.v[0],
        mask.v[1] ? b.v[1] : a.v[1],
        mask.v[2] ? b.v[2] : a.v[2],
        mask.v[3] ? b.v[3] : a.v[3]
    };
#endif
}

u32
simd_cmpeq_zero_mask_s32(simd_v4s32 a) {
#if USE_SSE4
    __m128i zero = _mm_setzero_si128();
    __m128i cmp = _mm_cmpeq_epi32(a.v, zero);
    return _mm_movemask_ps(_mm_castsi128_ps(cmp));
#else
    return ((a.v[0] == 0) ? 1 : 0) | ((a.v[1] == 0) ? 2 : 0) |
           ((a.v[2] == 0) ? 4 : 0) | ((a.v[3] == 0) ? 8 : 0);
#endif
}

simd_v4s32
simd_mul_s32(simd_v4s32 a, simd_v4s32 b) {
#if USE_SSE4
    return (simd_v4s32){_mm_mullo_epi32(a.v, b.v)};
#else
    return (simd_v4s32){{a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2], a.v[3] * b.v[3]}};
#endif
}

s32
simd_hsum_s32(simd_v4s32 a) {
#if USE_SSE4
    __m128i sum1 = _mm_hadd_epi32(a.v, a.v);
    __m128i sum2 = _mm_hadd_epi32(sum1, sum1);
    return _mm_cvtsi128_si32(sum2);
#else
    return a.v[0] + a.v[1] + a.v[2] + a.v[3];
#endif
}

simd_v4s32
simd_set_s32(s32 a, s32 b, s32 c, s32 d) {
#if USE_SSE4
    return (simd_v4s32){_mm_set_epi32(d, c, b, a)};
#else
    return (simd_v4s32){{a, b, c, d}};
#endif
}

#endif
