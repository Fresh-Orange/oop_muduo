
#ifndef _MUDUO_TIMERQUEUE_H_
#define _MUDUO_TIMERQUEUE_H_


#include "FdHandler.h"
#include "../base/Timer.h"

#include <set>
#include <vector>

namespace muduo
{
class EventLoop;


class TimerQueue : public FdHandler
{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();
    
    ///
    /// Schedules the callback to be run at given time,
    /// repeats if @c interval > 0.0.
    ///
    /// Must be thread safe. Usually be called from other threads.
    TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);
    TimerId addTimer(TimerCallback&& cb, Timestamp when, double interval);

    void cancel(TimerId timerId);

private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::pair<Timer*, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);
    
    virtual void handleRead(Timestamp& receiveTime);

    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop* loop_;
    TimerList timers_;
    
    ActiveTimerSet activeTimers_;
    bool callingExpiredTimers_; /* atomic */
    ActiveTimerSet cancelingTimers_;
};
}
#endif  // _MUDUO_TIMERQUEUE_H_

