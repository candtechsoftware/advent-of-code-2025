#pragma once
#include "base.h"

typedef union Vec2_f32 {
    struct
    {
        f32 x, y;
    };
    struct
    {
        f32 u, v;
    };
} Vec2_f32;

typedef union Vec2_f64 {
    struct
    {
        f64 x, y;
    };
    struct
    {
        f64 u, v;
    };
} Vec2_f64;

typedef struct Vec2_s16 {
    s16 x, y;
} Vec2_s16;

typedef struct Vec2_s32 Vec2_s32;
struct Vec2_s32 {
    s32 x, y;
};

typedef struct Vec2_s64 {
    s64 x, y;
} Vec2_s64;

typedef union Vec3_f32 {
    struct {
        f32 x;
        f32 y;
        f32 z;
    };
    struct {
        Vec2_f32 xy;
        f32      z0;
    };
} Vec3_f32;

typedef union Vec4_f32 {
    struct
    {
        f32 x, y, z, w;
    };
    struct {
        f32 x0, y0, x1, y1;
    };
    struct {
        Vec2_f32 p0, p1;
    };
    struct
    {
        f32 r, g, b, a;
    };
} Vec4_f32;

typedef struct Rng1_f32 {
    union {
        struct
        {
            f32 min;
            f32 max;
        };
        f32 v[2];
    };
} Rng1_f32;

typedef struct Rng1_u32 {
    union {
        struct
        {
            u32 min;
            u32 max;
        };
        u32 v[2];
    };
} Rng1_u32;

typedef struct Rng1_u64 {
    union {
        struct
        {
            u64 min;
            u64 max;
        };
        u64 v[2];
    };
} Rng1_u64;

typedef struct Rng2_s16 {
    Vec2_s16 min;
    Vec2_s16 max;
} Rng2_s16;

typedef struct Rng2_s32 {
    Vec2_s32 min;
    Vec2_s32 max;
} Rng2_s32;

typedef struct Rng2_f32 {
    union {
        struct {
            Vec2_f32 min;
            Vec2_f32 max;
        };

        struct {
            Vec2_f32 p0;
            Vec2_f32 p1;
        };
        struct {
            f32 x0, y0, x1, y1;
        };
    };
} Rng2_f32;

typedef struct Mat3x3_f32 {
    f32 m[3][3];
} Mat3x3_f32;

typedef struct Mat4x4_f32 {
    f32 m[4][4];
} Mat4x4_f32;

typedef struct Quaternion_f32 {
    f32 x, y, z, w;
} Quaternion_f32;

typedef Mat4x4_f32     Mat4_f32;
typedef Mat3x3_f32     Mat3_f32;
typedef Quaternion_f32 Quat_f32;

typedef Vec2_f32       Vec2;
typedef Vec3_f32       Vec3;
typedef Vec4_f32       Vec4;
typedef Mat3x3_f32     Mat3x3;
typedef Mat4x4_f32     Mat4x4;
typedef Quaternion_f32 Quaternion;

static inline Vec3 vec_3f32(f32 x, f32 y, f32 z) {
    Vec3 v = {x, y, z};
    return v;
}

#define v2f32(x, y)       ((Vec2_f32){{(x), (y)}})
#define v2s32(x, y)       ((Vec2_s32){(x), (y)})
#define v2s64(x, y)       ((Vec2_s64){(x), (y)})
#define v4f32(x, y, z, w) ((Vec4_f32){{(x), (y), (z), (w)}})

#define V2F32(x, y)       v2f32(x, y)
#define V2S32(x, y)       v2s32(x, y)
#define V2S64(x, y)       v2s64(x, y)
#define v3f32(x, y, z)    vec_3f32((x), (y), (z))
#define V4F32(x, y, z, w) v4f32(x, y, z, w)

#define r2f32(min, max)    ((Rng2_f32){min, max})
#define r2f32p(x, y, z, w) r2f32(v2f32((x), (y)), v2f32((z), (w)))
static inline Vec3 xform_3f32(Vec3 v, Mat3x3 m) {
    Vec3 result;
    result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
    result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
    result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
    return result;
}

static inline Mat3_f32 mat3_identity(void);
static inline Mat4_f32 mat4_identity(void);

static inline Mat4_f32 mat_4x4f32(f32 diagonal) {
    Mat4x4 result = {0};
    result.m[0][0] = diagonal;
    result.m[1][1] = diagonal;
    result.m[2][2] = diagonal;
    result.m[3][3] = diagonal;
    return result;
}

static inline Rng2_f32 intersect_2f32(Rng2_f32 a, Rng2_f32 b);

// Thank you john carmak and quake III 
static inline f32      fast_expf(f32 x) {
    union {
        float f;
        int   i;
    } u;
    u.i = (int)(12102203.0f * x + 1065353216.0f); // 12102203 = 1/ln(2) * (1 << 23)
    return u.f;
}
static inline f32 lerp(f32 current, f32 target, f32 rate, f32 dt) {
    f32 diff = target - current;
    return current + diff * (1.0f - fast_expf(-rate * dt));
}

// Ripped from Quake III 
static inline f32 fast_sqrt(f32 x) {
    union {
        f32 f;
        u32 i;
    } u;
    u.f = x;
    u.i = 0x5f3759df - (u.i >> 1); // magic number
    f32 y = u.f;
    y = y * (1.5f - 0.5f * x * y * y); // 1 iteration of Newton-Raphson
    return x * y;                      // invert to get sqrt
}

static inline f32 fast_maxf(f32 a, f32 b) {
    f32 diff = a - b;
    u32 mask = (u32)(*(u32 *)&diff >> 31); // 0xFFFFFFFF if diff < 0
    u32 result = (*(u32 *)&a & ~mask) | (*(u32 *)&b & mask);
    return *(f32 *)&result;
}

static inline float fast_minf(float a, float b) {
    f32 diff = a - b;
    u32 mask = (u32)(*(u32 *)&diff >> 31);
    u32 inv = ~mask;
    u32 result = (*(u32 *)&a & mask) | (*(u32 *)&b & inv);
    return *(f32 *)&result;
}

static inline Vec2 vec2f32_sub(Vec2 a, Vec2 b) {
    return (Vec2){
        .x = a.x - b.x,
        .y = a.y - b.y,
    };
}

static inline Vec4 vec4_lerp(Vec4 a, Vec4 b, f32 t) {
    Vec4 result;
    result.r = a.r + (b.r - a.r) * t;
    result.g = a.g + (b.g - a.g) * t;
    result.b = a.b + (b.b - a.b) * t;
    result.a = a.a + (b.a - a.a) * t;
    return result;
}

static inline Vec4 vec4_mul_alpha(Vec4 color, f32 alpha) {
    Vec4 result = color;
    result.a *= alpha;
    return result;
}

static inline Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

static inline f32 Abs(f32 x) {
    return x < 0 ? -x : x;
}

#define sqrt_f32(v)   fast_sqrt(v)
#define cbrt_f32(v)   cbrtf(v)
#define mod_f32(a, b) fmodf((a), (b))
#define pow_f32(b, e) powf((b), (e))
#define ceil_f32(v)   ceilf(v)
#define floor_f32(v)  floorf(v)
#define round_f32(v)  roundf(v)
#define abs_f32(v)    fabsf(v)

static inline u64 pow_u64(u64 base, u32 exp) {
    if (base == 2) return 1ULL << exp;
    u64 result = 1;
    for (u32 i = 0; i < exp; i++) result *= base;
    return result;
}

static inline u32 clz_u64(u64 n) {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanReverse64(&idx, n);
    return 63 - idx;
#else
    return __builtin_clzll(n);
#endif
}

static inline u32 digit_count_u64_impl(u64 n) {
    if (n == 0) return 1;
    u32 bits = 64 - clz_u64(n);
    u32 digits = (bits * 77) >> 8;
    if (n >= pow_u64(10, digits)) digits++;
    return digits;
}

#define digit_count_u64(n) digit_count_u64_impl(n)
