#if !USE_NEON && !USE_SSE4 && !USE_AVX2

static Simd_V16u8
simd_loadu_u8(const u8 *ptr) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = ptr[i];
    return r;
}

static Simd_V4f32
simd_loadu_f32(const f32 *ptr) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = ptr[i];
    return r;
}

static Simd_V4s32
simd_loadu_s32(const s32 *ptr) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = ptr[i];
    return r;
}

static Simd_V4u32
simd_loadu_u32(const u32 *ptr) {
    Simd_V4u32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = ptr[i];
    return r;
}

static void
simd_storeu_u8(u8 *ptr, Simd_V16u8 a) {
    for (u32 i = 0; i < 16; i++) ptr[i] = a.v[i];
}

static void
simd_storeu_f32(f32 *ptr, Simd_V4f32 a) {
    for (u32 i = 0; i < 4; i++) ptr[i] = a.v[i];
}

static void
simd_storeu_s32(s32 *ptr, Simd_V4s32 a) {
    for (u32 i = 0; i < 4; i++) ptr[i] = a.v[i];
}

static void
simd_storeu_u32(u32 *ptr, Simd_V4u32 a) {
    for (u32 i = 0; i < 4; i++) ptr[i] = a.v[i];
}

static Simd_V16u8
simd_set1_u8(u8 val) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = val;
    return r;
}

static Simd_V4f32
simd_set1_f32(f32 val) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = val;
    return r;
}

static Simd_V4s32
simd_set1_s32(s32 val) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = val;
    return r;
}

static Simd_V4u32
simd_set1_u32(u32 val) {
    Simd_V4u32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = val;
    return r;
}

static Simd_V4f32
simd_set_f32(f32 a, f32 b, f32 c, f32 d) {
    return (Simd_V4f32){{a, b, c, d}};
}

static Simd_V4s32
simd_set_s32(s32 a, s32 b, s32 c, s32 d) {
    return (Simd_V4s32){{a, b, c, d}};
}

static Simd_V4u32
simd_set_u32(u32 a, u32 b, u32 c, u32 d) {
    return (Simd_V4u32){{a, b, c, d}};
}

static Simd_V16u8
simd_zero_u8(void) {
    Simd_V16u8 r = {0};
    return r;
}

static Simd_V4f32
simd_zero_f32(void) {
    Simd_V4f32 r = {0};
    return r;
}

static Simd_V4s32
simd_zero_s32(void) {
    Simd_V4s32 r = {0};
    return r;
}

static Simd_V4u32
simd_zero_u32(void) {
    Simd_V4u32 r = {0};
    return r;
}

static Simd_V4f32
simd_add_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] + b.v[i];
    return r;
}

static Simd_V4f32
simd_sub_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] - b.v[i];
    return r;
}

static Simd_V4f32
simd_mul_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] * b.v[i];
    return r;
}

static Simd_V4f32
simd_div_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] / b.v[i];
    return r;
}

static Simd_V4s32
simd_add_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] + b.v[i];
    return r;
}

static Simd_V4s32
simd_sub_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] - b.v[i];
    return r;
}

static Simd_V4s32
simd_mul_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] * b.v[i];
    return r;
}

static Simd_V4f32
simd_fmadd_f32(Simd_V4f32 a, Simd_V4f32 b, Simd_V4f32 c) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] * b.v[i] + c.v[i];
    return r;
}

static Simd_V4f32
simd_min_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] < b.v[i] ? a.v[i] : b.v[i];
    return r;
}

static Simd_V4f32
simd_max_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] > b.v[i] ? a.v[i] : b.v[i];
    return r;
}

static Simd_V4s32
simd_min_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] < b.v[i] ? a.v[i] : b.v[i];
    return r;
}

static Simd_V4s32
simd_max_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] > b.v[i] ? a.v[i] : b.v[i];
    return r;
}

static Simd_V16u8
simd_min_u8(Simd_V16u8 a, Simd_V16u8 b) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = a.v[i] < b.v[i] ? a.v[i] : b.v[i];
    return r;
}

static Simd_V16u8
simd_max_u8(Simd_V16u8 a, Simd_V16u8 b) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = a.v[i] > b.v[i] ? a.v[i] : b.v[i];
    return r;
}

static Simd_V4f32
simd_abs_f32(Simd_V4f32 a) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] < 0 ? -a.v[i] : a.v[i];
    return r;
}

static Simd_V4f32
simd_neg_f32(Simd_V4f32 a) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = -a.v[i];
    return r;
}

static Simd_V4f32
simd_sqrt_f32(Simd_V4f32 a) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = sqrtf(a.v[i]);
    return r;
}

static Simd_V4s32
simd_abs_s32(Simd_V4s32 a) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] < 0 ? -a.v[i] : a.v[i];
    return r;
}

static Simd_V4s32
simd_neg_s32(Simd_V4s32 a) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = -a.v[i];
    return r;
}

static Simd_V16u8
simd_cmpeq_u8(Simd_V16u8 a, Simd_V16u8 b) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = a.v[i] == b.v[i] ? 0xFF : 0;
    return r;
}

static Simd_V4f32
simd_cmpeq_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) {
        u32 val = a.v[i] == b.v[i] ? 0xFFFFFFFF : 0;
        r.v[i] = *(f32 *)&val;
    }
    return r;
}

static Simd_V4s32
simd_cmpeq_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] == b.v[i] ? -1 : 0;
    return r;
}

static Simd_V16u8
simd_cmpgt_u8(Simd_V16u8 a, Simd_V16u8 b) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = a.v[i] > b.v[i] ? 0xFF : 0;
    return r;
}

static Simd_V4f32
simd_cmpgt_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) {
        u32 val = a.v[i] > b.v[i] ? 0xFFFFFFFF : 0;
        r.v[i] = *(f32 *)&val;
    }
    return r;
}

static Simd_V4s32
simd_cmpgt_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] > b.v[i] ? -1 : 0;
    return r;
}

static Simd_V4f32
simd_cmplt_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) {
        u32 val = a.v[i] < b.v[i] ? 0xFFFFFFFF : 0;
        r.v[i] = *(f32 *)&val;
    }
    return r;
}

static Simd_V4f32
simd_cmpge_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) {
        u32 val = a.v[i] >= b.v[i] ? 0xFFFFFFFF : 0;
        r.v[i] = *(f32 *)&val;
    }
    return r;
}

static Simd_V4f32
simd_cmple_f32(Simd_V4f32 a, Simd_V4f32 b) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) {
        u32 val = a.v[i] <= b.v[i] ? 0xFFFFFFFF : 0;
        r.v[i] = *(f32 *)&val;
    }
    return r;
}

static Simd_V16u8
simd_and_u8(Simd_V16u8 a, Simd_V16u8 b) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = a.v[i] & b.v[i];
    return r;
}

static Simd_V16u8
simd_or_u8(Simd_V16u8 a, Simd_V16u8 b) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = a.v[i] | b.v[i];
    return r;
}

static Simd_V16u8
simd_xor_u8(Simd_V16u8 a, Simd_V16u8 b) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = a.v[i] ^ b.v[i];
    return r;
}

static Simd_V16u8
simd_not_u8(Simd_V16u8 a) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = ~a.v[i];
    return r;
}

static Simd_V4s32
simd_and_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] & b.v[i];
    return r;
}

static Simd_V4s32
simd_or_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] | b.v[i];
    return r;
}

static Simd_V4s32
simd_xor_s32(Simd_V4s32 a, Simd_V4s32 b) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = a.v[i] ^ b.v[i];
    return r;
}

static Simd_V16u8
simd_andnot_u8(Simd_V16u8 a, Simd_V16u8 b) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = (~a.v[i]) & b.v[i];
    return r;
}

static u32
simd_movemask_u8(Simd_V16u8 a) {
    u32 mask = 0;
    for (u32 i = 0; i < 16; i++) {
        if (a.v[i] & 0x80) mask |= (1u << i);
    }
    return mask;
}

static u32
simd_movemask_f32(Simd_V4f32 a) {
    u32 mask = 0;
    for (u32 i = 0; i < 4; i++) {
        u32 bits = *(u32 *)&a.v[i];
        if (bits & 0x80000000) mask |= (1u << i);
    }
    return mask;
}

static u32
simd_movemask_s32(Simd_V4s32 a) {
    u32 mask = 0;
    for (u32 i = 0; i < 4; i++) {
        if (a.v[i] < 0) mask |= (1u << i);
    }
    return mask;
}

static Simd_V16u8
simd_blend_u8(Simd_V16u8 a, Simd_V16u8 b, Simd_V16u8 mask) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) r.v[i] = (mask.v[i] & 0x80) ? b.v[i] : a.v[i];
    return r;
}

static Simd_V4f32
simd_blend_f32(Simd_V4f32 a, Simd_V4f32 b, Simd_V4f32 mask) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) {
        u32 m = *(u32 *)&mask.v[i];
        r.v[i] = (m & 0x80000000) ? b.v[i] : a.v[i];
    }
    return r;
}

static Simd_V4s32
simd_blend_s32(Simd_V4s32 a, Simd_V4s32 b, Simd_V4s32 mask) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = (mask.v[i] < 0) ? b.v[i] : a.v[i];
    return r;
}

static s32
simd_hsum_s32(Simd_V4s32 a) {
    return a.v[0] + a.v[1] + a.v[2] + a.v[3];
}

static f32
simd_hsum_f32(Simd_V4f32 a) {
    return a.v[0] + a.v[1] + a.v[2] + a.v[3];
}

static s32
simd_hmin_s32(Simd_V4s32 a) {
    s32 m = a.v[0];
    for (u32 i = 1; i < 4; i++) if (a.v[i] < m) m = a.v[i];
    return m;
}

static s32
simd_hmax_s32(Simd_V4s32 a) {
    s32 m = a.v[0];
    for (u32 i = 1; i < 4; i++) if (a.v[i] > m) m = a.v[i];
    return m;
}

static f32
simd_hmin_f32(Simd_V4f32 a) {
    f32 m = a.v[0];
    for (u32 i = 1; i < 4; i++) if (a.v[i] < m) m = a.v[i];
    return m;
}

static f32
simd_hmax_f32(Simd_V4f32 a) {
    f32 m = a.v[0];
    for (u32 i = 1; i < 4; i++) if (a.v[i] > m) m = a.v[i];
    return m;
}

static Simd_V4f32
simd_cvt_s32_f32(Simd_V4s32 a) {
    Simd_V4f32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = (f32)a.v[i];
    return r;
}

static Simd_V4s32
simd_cvt_f32_s32(Simd_V4f32 a) {
    Simd_V4s32 r;
    for (u32 i = 0; i < 4; i++) r.v[i] = (s32)a.v[i];
    return r;
}

static u32
simd_ctz(u32 mask) {
    u32 count = 0;
    while ((mask & 1) == 0) {
        mask >>= 1;
        count++;
    }
    return count;
}

static u32
simd_clz(u32 mask) {
    u32 count = 0;
    for (s32 i = 31; i >= 0; i--) {
        if (mask & (1u << i)) break;
        count++;
    }
    return count;
}

static u32
simd_popcount(u32 mask) {
    u32 count = 0;
    while (mask) {
        count += mask & 1;
        mask >>= 1;
    }
    return count;
}

static b32
simd_any_true_u8(Simd_V16u8 a) {
    for (u32 i = 0; i < 16; i++) {
        if (a.v[i]) return 1;
    }
    return 0;
}

static b32
simd_all_true_u8(Simd_V16u8 a) {
    for (u32 i = 0; i < 16; i++) {
        if (a.v[i] != 0xFF) return 0;
    }
    return 1;
}

static Simd_V16u8
simd_shuffle_u8(Simd_V16u8 a, Simd_V16u8 indices) {
    Simd_V16u8 r;
    for (u32 i = 0; i < 16; i++) {
        u8 idx = indices.v[i];
        r.v[i] = (idx & 0x80) ? 0 : a.v[idx & 0x0F];
    }
    return r;
}

#endif
