#pragma once

#include "os.h"
#include "gfx/gfx.h"
#include "os_thread.h"

#if OS_WINDOWS
#    include "core/windows/os_core_win32.h"
#elif OS_LINUX || OS_MAC
#    include "core/posix/os_core_posix.h"
#endif
