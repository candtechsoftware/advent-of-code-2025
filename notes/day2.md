# Day 2 Results

## Part 1 

```
Part 1 (scalar): (time: 9 us)
Part 1 (SIMD):   (time: 11 us)
```

### SIMD WHY YOU SLOW? 
Too few iterations Each range only has a handful of valid prefixes per digit length.
Setup overhead - Creating SIMD vectors is not free. 

## Part 2 

```
Part 2 (scalar_slow): (time: 93785 us)
Part 2 (scalar_fast): (time: 7276 us)
Part 2 (simd):        (time: 34 us)
```

First shot was pretty brute force but obvious check every number in range. 
The btter thing to tod was for the fast version when doing it brute force you can 
see algo looks somthing like `pattern * (10^(k*(r-1)) + 10^(k*(r-2)) + ... + 10^k + 1)`
you can see the multiplier is a series of `(10^(k*r) - 1) / (10^k - 1)`

example, 3-digit patterns repeated 2x (6-digit numbers):
- multiplier = (10^6 - 1) / (10^3 - 1) = 999999 / 999 = 1001
- pattern=123 → 123 * 1001 = 123123
- pattern=456 → 456 * 1001 = 456456

Also a number like `1111` matches multiple pattern lengths:
- pattern_len=1, repeats=4: 1 * 1111 = 1111
- pattern_len=2, repeats=2: 11 * 101 = 1111
To avoid counting it twice, we skip patterns that are themselves repeated. 
So for `1111`, we only count when pattern=1 (since 11 is itself a repeated pattern of 1).

### SIMD IS FAST AF. 

Each digit length has many valid patterns:
- 2 digits: 9 patterns  = (1-9 repeated 2x)
- 4 digits: 90 patterns =  (10-99 repeated 2x) + 9 patterns (1-9 repeated 4x)
- 6 digits: 900 + 90 + 9 patterns
- etc.

With thousands of patterns to multiply and sum,
SIMD has enough work to amortize the setup cost.
Processing 4 patterns at a time with `simd_mul_s32` and `simd_hsum_s32` 

The biggest win was generating vs checking. 
SIMD gave another speed up on top top.

Things of note when thinking in a Vector/Simd way. Look at data holistically. 
Same thinking that gets you away from single allocations and an OOP style but thinking about 
data in a DOD way (awesome [video](https://www.youtube.com/watch?v=rX0ItVEVjHc) and another awesome [one](https://youtu.be/IroPQ150F6c?si=uZrZizYK-poJ3J3t)) 
like my transition from new/delete malloc/free and grouped lifetimes (arenas) it makes sense when you realize that you almost never operate on one item at a time. 
Like with 1111, 2222, 3333, 4444 - instead of checking each one, they're all just something * 1111.
SIMD forces you to ask "what's the same across these values?". Both times the SIMD optimizations were not too impresive 
but the algo that came about from thinking in that way revealed some easy optimizations. 
[Compiler explorer](https://godbolt.org/z/Per98dz6q)

