#if USE_AVX2 || USE_SSE4

static Simd_V16u8
simd_loadu_u8(const u8 *ptr) {
    return (Simd_V16u8){_mm_loadu_si128((const __m128i*)ptr)};
}

static Simd_V4f32
simd_loadu_f32(const f32 *ptr) {
    return (Simd_V4f32){_mm_loadu_ps(ptr)};
}

static Simd_V4s32
simd_loadu_s32(const s32 *ptr) {
    return (Simd_V4s32){_mm_loadu_si128((const __m128i*)ptr)};
}

static Simd_V4u32
simd_loadu_u32(const u32 *ptr) {
    return (Simd_V4u32){_mm_loadu_si128((const __m128i*)ptr)};
}

static void
simd_storeu_u8(u8 *ptr, Simd_V16u8 v) {
    _mm_storeu_si128((__m128i*)ptr, v.v);
}

static void
simd_storeu_f32(f32 *ptr, Simd_V4f32 v) {
    _mm_storeu_ps(ptr, v.v);
}

static void
simd_storeu_s32(s32 *ptr, Simd_V4s32 v) {
    _mm_storeu_si128((__m128i*)ptr, v.v);
}

static void
simd_storeu_u32(u32 *ptr, Simd_V4u32 v) {
    _mm_storeu_si128((__m128i*)ptr, v.v);
}

static Simd_V16u8
simd_set1_u8(u8 val) {
    return (Simd_V16u8){_mm_set1_epi8(val)};
}

static Simd_V4f32
simd_set1_f32(f32 val) {
    return (Simd_V4f32){_mm_set1_ps(val)};
}

static Simd_V4s32
simd_set1_s32(s32 val) {
    return (Simd_V4s32){_mm_set1_epi32(val)};
}

static Simd_V4u32
simd_set1_u32(u32 val) {
    return (Simd_V4u32){_mm_set1_epi32((s32)val)};
}

static Simd_V4f32
simd_set_f32(f32 a, f32 b, f32 c, f32 d) {
    return (Simd_V4f32){_mm_set_ps(d, c, b, a)};
}

static Simd_V4s32
simd_set_s32(s32 a, s32 b, s32 c, s32 d) {
    return (Simd_V4s32){_mm_set_epi32(d, c, b, a)};
}

static Simd_V4u32
simd_set_u32(u32 a, u32 b, u32 c, u32 d) {
    return (Simd_V4u32){_mm_set_epi32((s32)d, (s32)c, (s32)b, (s32)a)};
}

static Simd_V16u8
simd_zero_u8(void) {
    return (Simd_V16u8){_mm_setzero_si128()};
}

static Simd_V4f32
simd_zero_f32(void) {
    return (Simd_V4f32){_mm_setzero_ps()};
}

static Simd_V4s32
simd_zero_s32(void) {
    return (Simd_V4s32){_mm_setzero_si128()};
}

static Simd_V4u32
simd_zero_u32(void) {
    return (Simd_V4u32){_mm_setzero_si128()};
}

static Simd_V4f32
simd_add_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_add_ps(a.v, b.v)};
}

static Simd_V4f32
simd_sub_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_sub_ps(a.v, b.v)};
}

static Simd_V4f32
simd_mul_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_mul_ps(a.v, b.v)};
}

static Simd_V4f32
simd_div_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_div_ps(a.v, b.v)};
}

static Simd_V4s32
simd_add_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_add_epi32(a.v, b.v)};
}

static Simd_V4s32
simd_sub_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_sub_epi32(a.v, b.v)};
}

static Simd_V4s32
simd_mul_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_mullo_epi32(a.v, b.v)};
}

static Simd_V4f32
simd_fmadd_f32(Simd_V4f32 a, Simd_V4f32 b, Simd_V4f32 c) {
#if USE_AVX2
    return (Simd_V4f32){_mm_fmadd_ps(a.v, b.v, c.v)};
#else
    return (Simd_V4f32){_mm_add_ps(_mm_mul_ps(a.v, b.v), c.v)};
#endif
}

static Simd_V4f32
simd_neg_f32(Simd_V4f32 a) {
    return (Simd_V4f32){_mm_sub_ps(_mm_setzero_ps(), a.v)};
}

static Simd_V4s32
simd_neg_s32(Simd_V4s32 a) {
    return (Simd_V4s32){_mm_sub_epi32(_mm_setzero_si128(), a.v)};
}

static Simd_V4f32
simd_abs_f32(Simd_V4f32 a) {
    __m128i mask = _mm_set1_epi32(0x7FFFFFFF);
    return (Simd_V4f32){_mm_and_ps(a.v, _mm_castsi128_ps(mask))};
}

static Simd_V4s32
simd_abs_s32(Simd_V4s32 a) {
    return (Simd_V4s32){_mm_abs_epi32(a.v)};
}

static Simd_V4f32
simd_sqrt_f32(Simd_V4f32 a) {
    return (Simd_V4f32){_mm_sqrt_ps(a.v)};
}

static Simd_V16u8
simd_min_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){_mm_min_epu8(a.v, b.v)};
}

static Simd_V16u8
simd_max_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){_mm_max_epu8(a.v, b.v)};
}

static Simd_V4f32
simd_min_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_min_ps(a.v, b.v)};
}

static Simd_V4f32
simd_max_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_max_ps(a.v, b.v)};
}

static Simd_V4s32
simd_min_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_min_epi32(a.v, b.v)};
}

static Simd_V4s32
simd_max_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_max_epi32(a.v, b.v)};
}

static Simd_V16u8
simd_cmpeq_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){_mm_cmpeq_epi8(a.v, b.v)};
}

static Simd_V16u8
simd_cmpgt_u8(Simd_V16u8 a, Simd_V16u8 b) {
    __m128i bias = _mm_set1_epi8((char)0x80);
    __m128i a_biased = _mm_xor_si128(a.v, bias);
    __m128i b_biased = _mm_xor_si128(b.v, bias);
    return (Simd_V16u8){_mm_cmpgt_epi8(a_biased, b_biased)};
}

static Simd_V4f32
simd_cmpeq_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_cmpeq_ps(a.v, b.v)};
}

static Simd_V4f32
simd_cmpgt_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_cmpgt_ps(a.v, b.v)};
}

static Simd_V4f32
simd_cmplt_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_cmplt_ps(a.v, b.v)};
}

static Simd_V4f32
simd_cmpge_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_cmpge_ps(a.v, b.v)};
}

static Simd_V4f32
simd_cmple_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){_mm_cmple_ps(a.v, b.v)};
}

static Simd_V4s32
simd_cmpeq_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_cmpeq_epi32(a.v, b.v)};
}

static Simd_V4s32
simd_cmpgt_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_cmpgt_epi32(a.v, b.v)};
}

static Simd_V16u8
simd_and_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){_mm_and_si128(a.v, b.v)};
}

static Simd_V16u8
simd_or_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){_mm_or_si128(a.v, b.v)};
}

static Simd_V16u8
simd_xor_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){_mm_xor_si128(a.v, b.v)};
}

static Simd_V16u8
simd_not_u8(Simd_V16u8 a) {
    return (Simd_V16u8){_mm_xor_si128(a.v, _mm_set1_epi8(-1))};
}

static Simd_V16u8
simd_andnot_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){_mm_andnot_si128(a.v, b.v)};
}

static Simd_V4s32
simd_and_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_and_si128(a.v, b.v)};
}

static Simd_V4s32
simd_or_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_or_si128(a.v, b.v)};
}

static Simd_V4s32
simd_xor_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){_mm_xor_si128(a.v, b.v)};
}

static u32
simd_movemask_u8(Simd_V16u8 a) {
    return (u32)_mm_movemask_epi8(a.v);
}

static u32
simd_movemask_f32(Simd_V4f32 a) {
    return (u32)_mm_movemask_ps(a.v);
}

static u32
simd_movemask_s32(Simd_V4s32 a) {
    return (u32)_mm_movemask_ps(_mm_castsi128_ps(a.v));
}

static Simd_V16u8
simd_blend_u8(Simd_V16u8 a, Simd_V16u8 b, Simd_V16u8 mask) {
    return (Simd_V16u8){_mm_blendv_epi8(a.v, b.v, mask.v)};
}

static Simd_V4f32
simd_blend_f32(Simd_V4f32 a, Simd_V4f32 b, Simd_V4f32 mask) {
    return (Simd_V4f32){_mm_blendv_ps(a.v, b.v, mask.v)};
}

static Simd_V4s32
simd_blend_s32(Simd_V4s32 a, Simd_V4s32 b, Simd_V4s32 mask) {
    return (Simd_V4s32){_mm_blendv_epi8(a.v, b.v, mask.v)};
}

static s32
simd_hsum_s32(Simd_V4s32 a) {
    __m128i sum1 = _mm_hadd_epi32(a.v, a.v);
    __m128i sum2 = _mm_hadd_epi32(sum1, sum1);
    return _mm_cvtsi128_si32(sum2);
}

static f32
simd_hsum_f32(Simd_V4f32 a) {
    __m128 sum1 = _mm_hadd_ps(a.v, a.v);
    __m128 sum2 = _mm_hadd_ps(sum1, sum1);
    return _mm_cvtss_f32(sum2);
}

static s32
simd_hmin_s32(Simd_V4s32 a) {
    __m128i min1 = _mm_shuffle_epi32(a.v, _MM_SHUFFLE(2, 3, 0, 1));
    __m128i min2 = _mm_min_epi32(a.v, min1);
    __m128i min3 = _mm_shuffle_epi32(min2, _MM_SHUFFLE(1, 0, 3, 2));
    __m128i min4 = _mm_min_epi32(min2, min3);
    return _mm_cvtsi128_si32(min4);
}

static s32
simd_hmax_s32(Simd_V4s32 a) {
    __m128i max1 = _mm_shuffle_epi32(a.v, _MM_SHUFFLE(2, 3, 0, 1));
    __m128i max2 = _mm_max_epi32(a.v, max1);
    __m128i max3 = _mm_shuffle_epi32(max2, _MM_SHUFFLE(1, 0, 3, 2));
    __m128i max4 = _mm_max_epi32(max2, max3);
    return _mm_cvtsi128_si32(max4);
}

static f32
simd_hmin_f32(Simd_V4f32 a) {
    __m128 min1 = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 min2 = _mm_min_ps(a.v, min1);
    __m128 min3 = _mm_shuffle_ps(min2, min2, _MM_SHUFFLE(1, 0, 3, 2));
    __m128 min4 = _mm_min_ps(min2, min3);
    return _mm_cvtss_f32(min4);
}

static f32
simd_hmax_f32(Simd_V4f32 a) {
    __m128 max1 = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 max2 = _mm_max_ps(a.v, max1);
    __m128 max3 = _mm_shuffle_ps(max2, max2, _MM_SHUFFLE(1, 0, 3, 2));
    __m128 max4 = _mm_max_ps(max2, max3);
    return _mm_cvtss_f32(max4);
}

static Simd_V4f32
simd_cvt_s32_f32(Simd_V4s32 a) {
    return (Simd_V4f32){_mm_cvtepi32_ps(a.v)};
}

static Simd_V4s32
simd_cvt_f32_s32(Simd_V4f32 a) {
    return (Simd_V4s32){_mm_cvttps_epi32(a.v)};
}

static u32
simd_ctz(u32 mask) {
#if COMPILER_MSVC
    unsigned long index;
    _BitScanForward(&index, mask);
    return (u32)index;
#else
    return __builtin_ctz(mask);
#endif
}

static u32
simd_clz(u32 mask) {
#if COMPILER_MSVC
    unsigned long index;
    _BitScanReverse(&index, mask);
    return 31 - (u32)index;
#else
    return __builtin_clz(mask);
#endif
}

static u32
simd_popcount(u32 mask) {
#if COMPILER_MSVC
    return __popcnt(mask);
#else
    return __builtin_popcount(mask);
#endif
}

static b32
simd_any_true_u8(Simd_V16u8 a) {
    return _mm_movemask_epi8(a.v) != 0;
}

static b32
simd_all_true_u8(Simd_V16u8 a) {
    return _mm_movemask_epi8(a.v) == 0xFFFF;
}

static Simd_V16u8
simd_shuffle_u8(Simd_V16u8 a, Simd_V16u8 indices) {
    return (Simd_V16u8){_mm_shuffle_epi8(a.v, indices.v)};
}

#endif
