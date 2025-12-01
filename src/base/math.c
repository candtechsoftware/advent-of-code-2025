static inline Mat3_f32
mat3_identity(void) {
    Mat3_f32 result = {0};
    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    return result;
}

static inline Mat4_f32
mat4_identity(void) {
    Mat4_f32 result = {0};
    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;
    return result;
}
static inline Rng2_f32
intersect_2f32(Rng2_f32 a, Rng2_f32 b) {
    Rng2_f32 c;
    c.p0.x = Max(a.min.x, b.min.x);
    c.p0.y = Max(a.min.y, b.min.y);
    c.p1.x = Min(a.max.x, b.max.x);
    c.p1.y = Min(a.max.y, b.max.y);
    return c;
}
