

#include <sys/time.h>
#include <stdio.h>
#include <inttypes.h>

#include "Timestamp.h"

using namespace muduo;

static_assert(sizeof(Timestamp) == sizeof(int64_t), "sizeof(Timestamp) is not 8B");

std::string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf)-1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}

std::string Timestamp::toFormattedString(bool showMicroseconds) const
{
    char buf[32] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);

    if (showMicroseconds)
    {
        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                 microseconds);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}


///
/// 获得当前精确时间（1970年1月1日到现在的时间）,第二个参数一般都为空，
/// 因为我们一般都只是为了获得当前时间，而不用获得timezone的数值
/// int gettimeofday(struct timeval*tv, struct timezone *tz);
/// 
/// struct timeval{
///     long int tv_sec;  // 秒数
///     long int tv_usec; // 微秒数
/// }
/// 
/// struct timezone{
///     int tz_minuteswest; /* 和Greenwich 时间差了多少分钟 */
///     int tz_dsttime;     /* DST 时间的修正方式           */
/// }
/// 
Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

