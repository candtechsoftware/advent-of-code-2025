#include "base.c"
#include "profile.c"
#include "arena.c"
#include "base_tctx.c"
#include "string_core.c"
#include "format.c"
#include "cmd_line.c"
#include "logger.c"
#include "math.c"
#include "base_thread.c"

#if USE_NEON
#include "simd_neon.c"
#elif USE_AVX2 || USE_SSE4
#include "simd_x86.c"
#else
#include "simd_scalar.c"
#endif

#include "sort.c"
