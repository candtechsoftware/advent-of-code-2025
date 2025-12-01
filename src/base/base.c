static b32 
memory_match(void *a, void *b, u64 size) {
    u8 *a_ptr = (u8 *)a;
    u8 *b_ptr = (u8 *)b;
    for (u64 i = 0; i < size; i++) {
        if (a_ptr[i] != b_ptr[i]) {
            return 0;
        }
    }
    return 1;
}

static b32 
memory_is_zero(void *ptr, u64 size) {
    u8 *p = (u8 *)ptr;
    for (u64 i = 0; i < size; i++) {
        if (p[i] != 0) {
            return 0;
        }
    }
    return 1;
}

static Dense_Time 
dense_time_from_date_time(Date_Time date_time) {
    Dense_Time result = 0;
    result += date_time.year;
    result *= 12;
    result += date_time.mon;
    result *= 31;
    result += date_time.day;
    result *= 24;
    result += date_time.hour;
    result *= 60;
    result += date_time.min;
    result *= 61;
    result += date_time.sec;
    result *= 1000;
    result += date_time.msec;
    return result;
}

static Date_Time date_time_from_dense_time(Dense_Time time) {
    Date_Time result = {0};
    result.msec = time % 1000;
    time /= 1000;
    result.sec = time % 61;
    time /= 61;
    result.min = time % 60;
    time /= 60;
    result.hour = time % 24;
    time /= 24;
    result.day = time % 31;
    time /= 31;
    result.mon = time % 12;
    time /= 12;
    Assert(time <= max_U32);
    result.year = (u32)time;
    return (result);
}
