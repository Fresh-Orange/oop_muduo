
#ifndef _MUDUO_LOGGING_H_
#define _MUDUO_LOGGING_H_

#include "ThreadUtil.h"
#include "Logging.h"

#include <string>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

namespace muduo
{
class Logger{
public:
    static const int LEVEL_NONE     = (-1);
    static const int LEVEL_MIN      = 0;
    static const int LEVEL_FATAL    = 0;
    static const int LEVEL_ERROR    = 1;
    static const int LEVEL_WARN     = 2;
    static const int LEVEL_INFO     = 3;
    static const int LEVEL_DEBUG    = 4;
    static const int LEVEL_TRACE    = 5;
    static const int LEVEL_MAX      = 5;

    static int get_level(const char *levelname);
    static Logger* shared();

    std::string level_name();
    const char* get_level_name(int level);
    std::string output_name(){ return filename; }
    uint64_t rotate_size(){ return rotate_size_; }
    
private:
    FILE *fp; // 缺省是log.out，可以通过open函数设置
    char filename[PATH_MAX];
    int level_;

    MutexLock mutex_;
    
    struct{
        uint64_t w_curr;
        uint64_t w_total;
    }stats;
    uint64_t rotate_size_;
    
    void rotate();
    
public:
    Logger()
    {
        //fp = stdout;
        fp = fopen("./log.out", "w+");
        level_ = LEVEL_TRACE;
        filename[0] = '\0';
        rotate_size_ = 0;
        stats.w_curr = 0;
        stats.w_total = 0;
    }
    
    ~Logger(){ close(); }

    int level(){ return level_; }

    void set_level(int level){ this->level_ = level; }

    int open(FILE *fp, int level=LEVEL_DEBUG, bool is_threadsafe=false){ return 0; }
    
    int open(const char *filename, int level=LEVEL_DEBUG,
             bool is_threadsafe=false, uint64_t rotate_size=0){ return 0; }
    
    void close(){
        if(fp != stdin && fp != stdout){
            fclose(fp);
        }
    }

    int logv(int level, const char *fmt, va_list ap);

    //int trace(const char *fmt, ...);
    //int debug(const char *fmt, ...);
    //int info(const char *fmt, ...);
    //int warn(const char *fmt, ...);
    //int error(const char *fmt, ...);
    //int fatal(const char *fmt, ...);
};


int log_write(int level, const char *fmt, ...);

#define LOG_TRACE(fmt, args...)\
    log_write(muduo::Logger::LEVEL_TRACE, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define LOG_DEBUG(fmt, args...)\
    log_write(muduo::Logger::LEVEL_DEBUG, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define LOG_INFO(fmt, args...)\
    log_write(muduo::Logger::LEVEL_INFO,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define LOG_WARN(fmt, args...)\
    log_write(muduo::Logger::LEVEL_WARN,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define LOG_ERROR(fmt, args...)\
    log_write(muduo::Logger::LEVEL_ERROR, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define LOG_FATAL(fmt, args...)\
    log_write(muduo::Logger::LEVEL_FATAL, "%s(%d): " fmt, __FILE__, __LINE__, ##args)

#define CHECK_NOTNULL(val) \
if (nullptr == val) \
{ \
    assert(val != nullptr); \
    LOG_FATAL("val == NULL"); \
}

}

#endif  // _MUDUO_LOGGING_H_

