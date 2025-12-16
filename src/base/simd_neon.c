#if USE_NEON

static Simd_V16u8
simd_loadu_u8(const u8 *ptr) {
    return (Simd_V16u8){vld1q_u8(ptr)};
}

static Simd_V4f32
simd_loadu_f32(const f32 *ptr) {
    return (Simd_V4f32){vld1q_f32(ptr)};
}

static Simd_V4s32
simd_loadu_s32(const s32 *ptr) {
    return (Simd_V4s32){vld1q_s32(ptr)};
}

static Simd_V4u32
simd_loadu_u32(const u32 *ptr) {
    return (Simd_V4u32){vld1q_u32(ptr)};
}

static void
simd_storeu_u8(u8 *ptr, Simd_V16u8 v) {
    vst1q_u8(ptr, v.v);
}

static void
simd_storeu_f32(f32 *ptr, Simd_V4f32 v) {
    vst1q_f32(ptr, v.v);
}

static void
simd_storeu_s32(s32 *ptr, Simd_V4s32 v) {
    vst1q_s32(ptr, v.v);
}

static void
simd_storeu_u32(u32 *ptr, Simd_V4u32 v) {
    vst1q_u32(ptr, v.v);
}

static Simd_V16u8
simd_set1_u8(u8 val) {
    return (Simd_V16u8){vdupq_n_u8(val)};
}

static Simd_V4f32
simd_set1_f32(f32 val) {
    return (Simd_V4f32){vdupq_n_f32(val)};
}

static Simd_V4s32
simd_set1_s32(s32 val) {
    return (Simd_V4s32){vdupq_n_s32(val)};
}

static Simd_V4u32
simd_set1_u32(u32 val) {
    return (Simd_V4u32){vdupq_n_u32(val)};
}

static Simd_V4f32
simd_set_f32(f32 a, f32 b, f32 c, f32 d) {
    f32 vals[4] = {a, b, c, d};
    return (Simd_V4f32){vld1q_f32(vals)};
}

static Simd_V4s32
simd_set_s32(s32 a, s32 b, s32 c, s32 d) {
    s32 vals[4] = {a, b, c, d};
    return (Simd_V4s32){vld1q_s32(vals)};
}

static Simd_V4u32
simd_set_u32(u32 a, u32 b, u32 c, u32 d) {
    u32 vals[4] = {a, b, c, d};
    return (Simd_V4u32){vld1q_u32(vals)};
}

static Simd_V16u8
simd_zero_u8(void) {
    return (Simd_V16u8){vdupq_n_u8(0)};
}

static Simd_V4f32
simd_zero_f32(void) {
    return (Simd_V4f32){vdupq_n_f32(0.0f)};
}

static Simd_V4s32
simd_zero_s32(void) {
    return (Simd_V4s32){vdupq_n_s32(0)};
}

static Simd_V4u32
simd_zero_u32(void) {
    return (Simd_V4u32){vdupq_n_u32(0)};
}

static Simd_V4f32
simd_add_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vaddq_f32(a.v, b.v)};
}

static Simd_V4f32
simd_sub_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vsubq_f32(a.v, b.v)};
}

static Simd_V4f32
simd_mul_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vmulq_f32(a.v, b.v)};
}

static Simd_V4f32
simd_div_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vdivq_f32(a.v, b.v)};
}

static Simd_V4s32
simd_add_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){vaddq_s32(a.v, b.v)};
}

static Simd_V4s32
simd_sub_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){vsubq_s32(a.v, b.v)};
}

static Simd_V4s32
simd_mul_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){vmulq_s32(a.v, b.v)};
}

static Simd_V4f32
simd_fmadd_f32(Simd_V4f32 a, Simd_V4f32 b, Simd_V4f32 c) {
    return (Simd_V4f32){vfmaq_f32(c.v, a.v, b.v)};
}

static Simd_V4f32
simd_neg_f32(Simd_V4f32 a) {
    return (Simd_V4f32){vnegq_f32(a.v)};
}

static Simd_V4s32
simd_neg_s32(Simd_V4s32 a) {
    return (Simd_V4s32){vnegq_s32(a.v)};
}

static Simd_V4f32
simd_abs_f32(Simd_V4f32 a) {
    return (Simd_V4f32){vabsq_f32(a.v)};
}

static Simd_V4s32
simd_abs_s32(Simd_V4s32 a) {
    return (Simd_V4s32){vabsq_s32(a.v)};
}

static Simd_V4f32
simd_sqrt_f32(Simd_V4f32 a) {
    return (Simd_V4f32){vsqrtq_f32(a.v)};
}

static Simd_V16u8
simd_min_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){vminq_u8(a.v, b.v)};
}

static Simd_V16u8
simd_max_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){vmaxq_u8(a.v, b.v)};
}

static Simd_V4f32
simd_min_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vminq_f32(a.v, b.v)};
}

static Simd_V4f32
simd_max_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vmaxq_f32(a.v, b.v)};
}

static Simd_V4s32
simd_min_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){vminq_s32(a.v, b.v)};
}

static Simd_V4s32
simd_max_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){vmaxq_s32(a.v, b.v)};
}

static Simd_V16u8
simd_cmpeq_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){vceqq_u8(a.v, b.v)};
}

static Simd_V16u8
simd_cmpgt_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){vcgtq_u8(a.v, b.v)};
}

static Simd_V4f32
simd_cmpeq_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vreinterpretq_f32_u32(vceqq_f32(a.v, b.v))};
}

static Simd_V4f32
simd_cmpgt_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vreinterpretq_f32_u32(vcgtq_f32(a.v, b.v))};
}

static Simd_V4f32
simd_cmplt_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vreinterpretq_f32_u32(vcltq_f32(a.v, b.v))};
}

static Simd_V4f32
simd_cmpge_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vreinterpretq_f32_u32(vcgeq_f32(a.v, b.v))};
}

static Simd_V4f32
simd_cmple_f32(Simd_V4f32 a, Simd_V4f32 b) {
    return (Simd_V4f32){vreinterpretq_f32_u32(vcleq_f32(a.v, b.v))};
}

static Simd_V4s32
simd_cmpeq_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){vreinterpretq_s32_u32(vceqq_s32(a.v, b.v))};
}

static Simd_V4s32
simd_cmpgt_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){vreinterpretq_s32_u32(vcgtq_s32(a.v, b.v))};
}

static Simd_V16u8
simd_and_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){vandq_u8(a.v, b.v)};
}

static Simd_V16u8
simd_or_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){vorrq_u8(a.v, b.v)};
}

static Simd_V16u8
simd_xor_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){veorq_u8(a.v, b.v)};
}

static Simd_V16u8
simd_not_u8(Simd_V16u8 a) {
    return (Simd_V16u8){vmvnq_u8(a.v)};
}

static Simd_V16u8
simd_andnot_u8(Simd_V16u8 a, Simd_V16u8 b) {
    return (Simd_V16u8){vbicq_u8(b.v, a.v)};
}

static Simd_V4s32
simd_and_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){vandq_s32(a.v, b.v)};
}

static Simd_V4s32
simd_or_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){vorrq_s32(a.v, b.v)};
}

static Simd_V4s32
simd_xor_s32(Simd_V4s32 a, Simd_V4s32 b) {
    return (Simd_V4s32){veorq_s32(a.v, b.v)};
}

static u32
simd_movemask_u8(Simd_V16u8 a) {
    static const u8 shift_vals[16] = {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7};
    uint8x16_t shift = vld1q_u8(shift_vals);
    uint8x16_t shifted = vshrq_n_u8(a.v, 7);
    shifted = vshlq_u8(shifted, vreinterpretq_s8_u8(shift));
    uint8x8_t lo = vget_low_u8(shifted);
    uint8x8_t hi = vget_high_u8(shifted);
    u8 lo_sum = vaddv_u8(lo);
    u8 hi_sum = vaddv_u8(hi);
    return (u32)lo_sum | ((u32)hi_sum << 8);
}

static u32
simd_movemask_f32(Simd_V4f32 a) {
    uint32x4_t shifted = vshrq_n_u32(vreinterpretq_u32_f32(a.v), 31);
    u32 mask = vgetq_lane_u32(shifted, 0) |
               (vgetq_lane_u32(shifted, 1) << 1) |
               (vgetq_lane_u32(shifted, 2) << 2) |
               (vgetq_lane_u32(shifted, 3) << 3);
    return mask;
}

static u32
simd_movemask_s32(Simd_V4s32 a) {
    uint32x4_t shifted = vshrq_n_u32(vreinterpretq_u32_s32(a.v), 31);
    u32 mask = vgetq_lane_u32(shifted, 0) |
               (vgetq_lane_u32(shifted, 1) << 1) |
               (vgetq_lane_u32(shifted, 2) << 2) |
               (vgetq_lane_u32(shifted, 3) << 3);
    return mask;
}

static Simd_V16u8
simd_blend_u8(Simd_V16u8 a, Simd_V16u8 b, Simd_V16u8 mask) {
    return (Simd_V16u8){vbslq_u8(mask.v, b.v, a.v)};
}

static Simd_V4f32
simd_blend_f32(Simd_V4f32 a, Simd_V4f32 b, Simd_V4f32 mask) {
    return (Simd_V4f32){vbslq_f32(vreinterpretq_u32_f32(mask.v), b.v, a.v)};
}

static Simd_V4s32
simd_blend_s32(Simd_V4s32 a, Simd_V4s32 b, Simd_V4s32 mask) {
    return (Simd_V4s32){vbslq_s32(vreinterpretq_u32_s32(mask.v), b.v, a.v)};
}

static s32
simd_hsum_s32(Simd_V4s32 a) {
    return vaddvq_s32(a.v);
}

static f32
simd_hsum_f32(Simd_V4f32 a) {
    return vaddvq_f32(a.v);
}

static s32
simd_hmin_s32(Simd_V4s32 a) {
    return vminvq_s32(a.v);
}

static s32
simd_hmax_s32(Simd_V4s32 a) {
    return vmaxvq_s32(a.v);
}

static f32
simd_hmin_f32(Simd_V4f32 a) {
    return vminvq_f32(a.v);
}

static f32
simd_hmax_f32(Simd_V4f32 a) {
    return vmaxvq_f32(a.v);
}

static Simd_V4f32
simd_cvt_s32_f32(Simd_V4s32 a) {
    return (Simd_V4f32){vcvtq_f32_s32(a.v)};
}

static Simd_V4s32
simd_cvt_f32_s32(Simd_V4f32 a) {
    return (Simd_V4s32){vcvtq_s32_f32(a.v)};
}

static u32
simd_ctz(u32 mask) {
    return __builtin_ctz(mask);
}

static u32
simd_clz(u32 mask) {
    return __builtin_clz(mask);
}

static u32
simd_popcount(u32 mask) {
    return __builtin_popcount(mask);
}

static b32
simd_any_true_u8(Simd_V16u8 a) {
    return vmaxvq_u8(a.v) != 0;
}

static b32
simd_all_true_u8(Simd_V16u8 a) {
    return vminvq_u8(a.v) != 0;
}

static Simd_V16u8
simd_shuffle_u8(Simd_V16u8 a, Simd_V16u8 indices) {
    return (Simd_V16u8){vqtbl1q_u8(a.v, indices.v)};
}

#endif
