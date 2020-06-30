
#ifndef _MUDUO_TIMER_H_
#define _MUDUO_TIMER_H_

#include "copyable.h"
#include "Timestamp.h"
#include "Atomic.h"
#include <functional>

namespace muduo
{

class TimerQueue;

/// 使用function类模板定义回调函数类型TimerCallback
typedef std::function<void()> TimerCallback;

///
/// Internal class for timer event.
///
class Timer : public noncopyable
{
public:
    Timer(const TimerCallback& cb, Timestamp when, double interval)
    : callback_(cb),
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0),
      sequence_(s_numCreated_.incrementAndGet())
    { }

    Timer(TimerCallback&& cb, Timestamp when, double interval)
    : callback_(std::move(cb)), // 显式移动构造,将cb中的资源转移到callback_
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0),
      sequence_(s_numCreated_.incrementAndGet())
    { }

    void run() const
    {
        callback_();
    }

    Timestamp expiration() const  { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(Timestamp now);

    static int64_t numCreated() { return s_numCreated_.get(); }

private:
    const TimerCallback callback_;// 定时器超时后的回调函数
    Timestamp expiration_;        // 下一次的超时的时间
    const double interval_;       // 超时时间间隔，如果是一次性定时器，该值为0
    const bool repeat_;           // 是否是周期定时器
    const int64_t sequence_;      // 定时器的序列号

    static AtomicInt64 s_numCreated_; // 定时器计数，当前已经创建的定时器数量
};

///
/// An opaque identifier, for canceling Timer.
///
class TimerId : public copyable
{
public:
    TimerId()
    : timer_(NULL), sequence_(0){}

    TimerId(Timer* timer, int64_t seq)
    : timer_(timer), sequence_(seq){}

    // default copy-ctor, dtor and assignment are okay

    friend class TimerQueue;

private:
    Timer* timer_;
    int64_t sequence_;
};

}

#endif  // _MUDUO_TIMER_H_

