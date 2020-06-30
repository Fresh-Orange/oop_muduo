
#ifndef _MUDUO_TIMER_H_
#define _MUDUO_TIMER_H_

#include "copyable.h"
#include "Timestamp.h"
#include "Atomic.h"
#include <functional>

namespace muduo
{

class TimerQueue;

/// ʹ��function��ģ�嶨��ص���������TimerCallback
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
    : callback_(std::move(cb)), // ��ʽ�ƶ�����,��cb�е���Դת�Ƶ�callback_
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
    const TimerCallback callback_;// ��ʱ����ʱ��Ļص�����
    Timestamp expiration_;        // ��һ�εĳ�ʱ��ʱ��
    const double interval_;       // ��ʱʱ�����������һ���Զ�ʱ������ֵΪ0
    const bool repeat_;           // �Ƿ������ڶ�ʱ��
    const int64_t sequence_;      // ��ʱ�������к�

    static AtomicInt64 s_numCreated_; // ��ʱ����������ǰ�Ѿ������Ķ�ʱ������
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

