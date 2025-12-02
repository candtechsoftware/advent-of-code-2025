#if USE_NEON

simd_v4s32
simd_load4_s32(s32 *ptr) {
    return (simd_v4s32){vld1q_s32(ptr)};
}

void
simd_store4_s32(s32 *ptr, simd_v4s32 val) {
    vst1q_s32(ptr, val.v);
}

simd_v4s32
simd_set1_s32(s32 val) {
    return (simd_v4s32){vdupq_n_s32(val)};
}

simd_v4s32
simd_add_s32(simd_v4s32 a, simd_v4s32 b) {
    return (simd_v4s32){vaddq_s32(a.v, b.v)};
}

simd_v4s32
simd_sub_s32(simd_v4s32 a, simd_v4s32 b) {
    return (simd_v4s32){vsubq_s32(a.v, b.v)};
}

simd_v4s32
simd_neg_s32(simd_v4s32 a) {
    return (simd_v4s32){vnegq_s32(a.v)};
}

simd_v4s32
simd_blend_s32(simd_v4s32 a, simd_v4s32 b, simd_v4s32 mask) {
    return (simd_v4s32){vbslq_s32(vreinterpretq_u32_s32(mask.v), b.v, a.v)};
}

u32
simd_cmpeq_zero_mask_s32(simd_v4s32 a) {
    int32x4_t zero = vdupq_n_s32(0);
    uint32x4_t cmp = vceqq_s32(a.v, zero);
    u32 mask = vgetq_lane_u32(cmp, 0) | (vgetq_lane_u32(cmp, 1) << 1) |
               (vgetq_lane_u32(cmp, 2) << 2) | (vgetq_lane_u32(cmp, 3) << 3);
    return mask;
}

simd_v4s32
simd_mul_s32(simd_v4s32 a, simd_v4s32 b) {
    return (simd_v4s32){vmulq_s32(a.v, b.v)};
}

s32
simd_hsum_s32(simd_v4s32 a) {
    return vaddvq_s32(a.v);
}

simd_v4s32
simd_set_s32(s32 a, s32 b, s32 c, s32 d) {
    s32 vals[4] = {a, b, c, d};
    return (simd_v4s32){vld1q_s32(vals)};
}

#endif
