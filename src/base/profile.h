#pragma once

#include "base.h"

#if PROFILE_MODE

void prof_open(char *name);
void prof_close(void);
void prof_thread_begin(void);
void prof_thread_end(void);
void prof_thread_flush(void);
void prof_begin(const u8 *name, u32 len);
void prof_end(void);

#endif
