# Day 2 Results

> [!IMPORTANT]
> The timings are kinda ghetto and don't really take this as an scientific fact just a someone trying to learn/share. Things could definetly be wrong!

```
Part 1 (scalar): (time: 6 us)
Part 1 (SIMD):   (time: 9 us)
```

## The Problem

Find all "doubled" numbers in ranges and sum them. A doubled number has the first half of digits equal to the second half: `11`, `1212`, `123123`, etc.

## SIMD Ops Added

Added three new operations to the SIMD library:

- `simd_mul_s32(a, b)` - multiply two vectors element-wise
- `simd_hsum_s32(a)` - horizontal sum (reduce 4 values to 1)
- `simd_set_s32(a, b, c, d)` - create vector from 4 individual values

## The Algorithm

Instead of checking every number in a range, generate doubled numbers directly.

A doubled number with `2k` digits is: `prefix * 10^k + prefix = prefix * (10^k + 1)`

For k=2 (4-digit numbers):
- prefix=12 → 12 * 101 = 1212
- prefix=45 → 45 * 101 = 4545

So scalar loops through prefixes one at a time, SIMD processes 4 prefixes at once.

## Why SIMD Was Slower

1. **Too few iterations** - Each range only has a handful of valid prefixes per digit length. SIMD needs thousands of iterations to shine, not 5-10.

2. **Horizontal sum is expensive** - After multiplying 4 values in parallel, we must reduce them to one scalar. This `hadd` instruction negates much of the parallel gain.

3. **Setup overhead** - Creating SIMD vectors has overhead that dominates when the loop body is tiny.

4. **Scalar is already fast** - A single multiply is ~1-2 cycles. The compiler unrolls and optimizes it heavily.

## When SIMD Would Win

If you had ranges like `1-9999999999` with millions of prefixes to sum, SIMD would be faster. But with sparse ranges and small prefix counts, scalar wins.

## Compiler Explorer Code

```c
#include <arm_neon.h>
#include <stdint.h>

typedef int32_t s32;
typedef uint32_t u32;
typedef uint64_t u64;

// SIMD type
typedef struct { int32x4_t v; } simd_v4s32;

// SIMD ops
static inline simd_v4s32 simd_set1_s32(s32 val) {
    return (simd_v4s32){vdupq_n_s32(val)};
}

static inline simd_v4s32 simd_set_s32(s32 a, s32 b, s32 c, s32 d) {
    s32 vals[4] = {a, b, c, d};
    return (simd_v4s32){vld1q_s32(vals)};
}

static inline simd_v4s32 simd_mul_s32(simd_v4s32 a, simd_v4s32 b) {
    return (simd_v4s32){vmulq_s32(a.v, b.v)};
}

static inline s32 simd_hsum_s32(simd_v4s32 a) {
    return vaddvq_s32(a.v);
}

// Sum prefixes lo..hi multiplied by mult using SIMD
u64 sum_prefixes_simd(u64 lo, u64 hi, s32 mult) {
    u64 sum = 0;
    u64 count = hi - lo + 1;
    u64 p = lo;

    simd_v4s32 multiplier = simd_set1_s32(mult);

    u64 i = 0;
    for (; i + 4 <= count; i += 4) {
        simd_v4s32 prefixes = simd_set_s32((s32)p, (s32)(p+1), (s32)(p+2), (s32)(p+3));
        simd_v4s32 products = simd_mul_s32(prefixes, multiplier);
        sum += (u64)simd_hsum_s32(products);
        p += 4;
    }

    for (; i < count; i++) {
        sum += p * mult;
        p++;
    }

    return sum;
}

// Sum prefixes lo..hi multiplied by mult using scalar
u64 sum_prefixes_scalar(u64 lo, u64 hi, s32 mult) {
    u64 sum = 0;
    for (u64 p = lo; p <= hi; p++) {
        sum += p * mult;
    }
    return sum;
}
```

x86 SSE version:
```c
#include <immintrin.h>
#include <stdint.h>

typedef int32_t s32;
typedef uint32_t u32;
typedef uint64_t u64;

// SIMD type
typedef struct { __m128i v; } simd_v4s32;

// SIMD ops
static inline simd_v4s32 simd_set1_s32(s32 val) {
    return (simd_v4s32){_mm_set1_epi32(val)};
}

static inline simd_v4s32 simd_set_s32(s32 a, s32 b, s32 c, s32 d) {
    return (simd_v4s32){_mm_set_epi32(d, c, b, a)};
}

static inline simd_v4s32 simd_mul_s32(simd_v4s32 a, simd_v4s32 b) {
    return (simd_v4s32){_mm_mullo_epi32(a.v, b.v)};
}

static inline s32 simd_hsum_s32(simd_v4s32 a) {
    __m128i sum1 = _mm_hadd_epi32(a.v, a.v);
    __m128i sum2 = _mm_hadd_epi32(sum1, sum1);
    return _mm_cvtsi128_si32(sum2);
}

// Sum prefixes lo..hi multiplied by mult using SIMD
u64 sum_prefixes_simd(u64 lo, u64 hi, s32 mult) {
    u64 sum = 0;
    u64 count = hi - lo + 1;
    u64 p = lo;

    simd_v4s32 multiplier = simd_set1_s32(mult);

    u64 i = 0;
    for (; i + 4 <= count; i += 4) {
        simd_v4s32 prefixes = simd_set_s32((s32)p, (s32)(p+1), (s32)(p+2), (s32)(p+3));
        simd_v4s32 products = simd_mul_s32(prefixes, multiplier);
        sum += (u64)simd_hsum_s32(products);
        p += 4;
    }

    for (; i < count; i++) {
        sum += p * mult;
        p++;
    }

    return sum;
}

// Sum prefixes lo..hi multiplied by mult using scalar
u64 sum_prefixes_scalar(u64 lo, u64 hi, s32 mult) {
    u64 sum = 0;
    for (u64 p = lo; p <= hi; p++) {
        sum += p * mult;
    }
    return sum;
}
```

- [Compiler explorer NEON](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGe1wAyeAyYAHI%2BAEaYxBIapAAOqAqETgwe3r56KWmOAkEh4SxRMVxxdpgOaUIETMQEmT5%2BQbaY9hlNzQQFYZHRcQlSNXUEFU0EpVGx8YkKDU0tue0jvf1lg%2BUAlBaorsTI7BwEmCxJBpsA1MrEmOgA%2BhqHJv4Abqh4LAD0L6wAdE/IzxermeTDwLHQJwAbqRsEwMlQIE8Xm8Pp8TAB2Kxwx4AEWeAFZUU8ACLgu5PfxuAQCJ7ASQJOgsFIETBJB7vQnAmEMOGA7CkwkkskA87ky6oG4jYheBxPAIAJVQTGp6Fe7zOjF%2Bh3OTAA%2BgBJBLoAgCACOeMhdzuD2AmDR0KusMY8NwdAe%2BJuADkgZh/NgaBAEBqdXcoYj0M9CMh0YdMQARJ4EW7AwEPADyeGQi3tyJRAHYHrC/S9EU9A%2BiY3ikUm3Sm0%2B7M56rT6/Z8NWqoZqNVrkbLgGbMJasYHIQBaEgEBgPAwYYlCqiYJXcrhAp5eOHIbNPMz%2BI4gdFPL7iyuVyxVmsg4F1%2BuN5s/FsEttdtE9gfd9GMocEo0/Y3EE2o%2BiCG7bvOzaHrmvIWgGQagPuU7PJgdATDhqCHMcpxAs8wCYAA7kIAgJGueIEPitAQvuHYmFysr4hSVI0nSVznj%2BWz7BBGYrMQhFPMQACymBuIe9L/j69FPMclFKkiGhUWBoBuBAszBk8UqseyN5IZqepGruhwPKxHE/px0KHDZDzwvxQkidMYlPJJ/qbshSIvOhKr%2BjpYb6SGKHJqZ6ZPNmSG5h4yAAPIEE8mCcApXg%2BVB2nhgAvkaLDbMQPmKfJRlxQi2bABFTAAG50ChFqMSyiU%2BdakVRdx%2BWpWOkbyglKUsIpVWNgqqXhellV/uFKI1dWdXYKFynMa13Htb1Rp9WeA3Pn1I35alJVlRV02dXJq4dV%2Be7pUZh3bW4AByDCTOk6A3B4hzIGC0rhS82CYJwJgJJ29KgnZGSXkw0X/k8IZGdF7IjgQCLJfdr1mMyFLCIctxQ5CIZImF7FMLlqWlSlCWQ2R/hXcAkFtJ2qAsDWIB1Y1gPNS9YFdT1MlA1gJhHWm0mg5d4O%2BFD6qQ9lsNvWjCNXOaCPo8xrk8eVgmHQQIlpRlMiXb%2BqAAPLIH8THJWYcDM6zFlwzR8NCfWCgKEp4N0/jMm9QLLOpezEAKBzXNi%2B5vMI6jVHsrx4siYiEvSzLcsy%2BdPm%2BSjQIqxDavs2Dq0iw1DxJZzkt0/DBlc1hKZMQrJji7dksa8ZWvPalJMG3TXu1a9jOQ6r0KhVRZufdbPO27bkP0zzisO4L2uYy7mvu9r2FHLr8t%2B9xAd8cHHG0aHIkO0bstQ8jM4A/HFHe4xaePDNBdPBT3O09bIekztkd27HSu92HTEDzA9yx1L8ck1hE8MwqRPMZgC8SbOBDZcrkOk99kKSzh%2BMXLMjFq%2B%2BnxDLyGe8sqMZM0kR4B0PoMMmQzHRPfJMHNnYHB9u/c2ltCbXCXivYBm8q5awYO/aEm9k7b1TlbEgBBYRGGNrCAgpwPiUHMsAeuuNiZiMkRoEq8x/AACV5iuCwK4Nw1tqJ0NgmBOysCEFuCQdfFBaDPZYPxiYvBBDkGoLQcghBiikFaJQcQ/BFjT6ELQf4AB2B9GEOwVqE%2BhJDcEIP8aQoBWCIlkMIaA9xJh/C6P0aI/AwSvTOOlFoJgUQOBTEYMw3xDjsDOPRhGZIx9SB5iMHgX%2BtAgk50RMEiwITMQy0lMwLACSXQpJNGkjJpTsk5L4eoDQURkBcAANYWAgVgDgtJUB3MoCwUwJgdnyG4I8eo1TakyDYWCTgEDIDZNMFMvYXBJBcAeBoKQ7S8C/IMFQO5RhJDvPebgYALASCjPGRYJ53BFAKHmNgZgkhKBPKJG8BQVSICsIBPMvAkgPnLIQGYCArAkB4BqKwK0eBJAKG%2BaoIEryHh4k2BwPAzBpBcAgCCDJEBAA%3D%3D)
