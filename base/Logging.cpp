
#include "Logging.h"
#include <sys/time.h>
#include <cstring>

namespace muduo
{

#define LEVEL_NAME_LEN  8
#define LOG_BUF_LEN     4096

static Logger logger;

Logger* Logger::shared(){return &logger;}

std::string Logger::level_name(){
    switch(level_){
        case Logger::LEVEL_FATAL:
            return "fatal";
        case Logger::LEVEL_ERROR:
            return "error";
        case Logger::LEVEL_WARN:
            return "warn";
        case Logger::LEVEL_INFO:
            return "info";
        case Logger::LEVEL_DEBUG:
            return "debug";
        case Logger::LEVEL_TRACE:
            return "trace";
    }
    return "";
}

const char* Logger::get_level_name(int level){
    switch(level){
        case Logger::LEVEL_FATAL:
            return "[FATAL] ";
        case Logger::LEVEL_ERROR:
            return "[ERROR] ";
        case Logger::LEVEL_WARN:
            return "[WARN ] ";
        case Logger::LEVEL_INFO:
            return "[INFO ] ";
        case Logger::LEVEL_DEBUG:
            return "[DEBUG] ";
        case Logger::LEVEL_TRACE:
            return "[TRACE] ";
    }
    return "";
}

void Logger::rotate(){
    fclose(fp);
    char newpath[PATH_MAX];
    time_t time;
    struct timeval tv;
    struct tm *tm;
    gettimeofday(&tv, NULL);
    time = tv.tv_sec;
    tm = localtime(&time);
    sprintf(newpath, "%s.%04d%02d%02d-%02d%02d%02d",
    this->filename,
    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
    tm->tm_hour, tm->tm_min, tm->tm_sec);

    //printf("rename %s => %s\n", this->filename, newpath);
    int ret = rename(this->filename, newpath);
    if(ret == -1){
        return;
    }
    fp = fopen(this->filename, "a");
    if(fp == NULL){
        return;
    }
    stats.w_curr = 0;
}

int Logger::logv(int level, const char *fmt, va_list ap){
    if(logger.level_ < level){
        return 0;
    }

    char buf[LOG_BUF_LEN];
    int len;
    char *ptr = buf;

    time_t time;
    struct timeval tv;
    struct tm *tm;
    gettimeofday(&tv, NULL);
    time = tv.tv_sec;
    tm = localtime(&time);
    /* %3ld 在数值位数超过3位的时候不起作用, 所以这里转成int */
    len = sprintf(ptr, "%04d-%02d-%02d %02d:%02d:%02d.%03d ",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec/1000));
    if(len < 0){
        return -1;
    }
    ptr += len;

    memcpy(ptr, get_level_name(level), LEVEL_NAME_LEN);
    ptr += LEVEL_NAME_LEN;

    int space = sizeof(buf) - (ptr - buf) - 10;
    len = vsnprintf(ptr, space, fmt, ap);
    if(len < 0){
        return -1;
    }
    ptr += len > space? space : len;
    *ptr++ = '\n';
    *ptr = '\0';

    len = ptr - buf;

    MutexLockGuard lock(mutex_); // 加入互斥锁
    fwrite(buf, len, 1, this->fp);
    fflush(this->fp);
    stats.w_curr += len;
    stats.w_total += len;
    if(rotate_size_ > 0 && stats.w_curr > rotate_size_){
        this->rotate();
    }

    if (level == LEVEL_FATAL)
    {
        abort();
    }
    return len;
}


int log_write(int level, const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    int ret = muduo::logger.logv(level, fmt, ap);
    va_end(ap);
    return ret;
}

}

