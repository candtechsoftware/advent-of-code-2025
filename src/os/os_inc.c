#if defined(__linux__)
#    ifndef _GNU_SOURCE
#        define _GNU_SOURCE
#    endif
#endif

#include "gfx/gfx.c"

#if defined(_WIN32) || defined(_WIN64)
#    include "core/windows/os_core_win32.c"
#    include "gfx/gfx_win32.c"
#elif defined(__linux__) || defined(__APPLE__)
#    include "core/posix/os_core_posix.c"
#    include "core/posix/os_core_posix_entry.c"
#endif

#if defined(__linux__)
#    include "gfx/gfx_x11.c"
#elif defined(__APPLE__)
#    ifdef __OBJC__
#        include "gfx/gfx_mac.m"
#    endif
#endif
