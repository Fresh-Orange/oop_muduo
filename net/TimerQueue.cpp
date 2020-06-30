
#include "TimerQueue.h"
#include "EventLoop.h"
#include "../base/Socket.h"

#include <sys/timerfd.h>
#include <unistd.h> // read, close, open
#include <string.h>

using namespace muduo;

namespace muduo
{

int createTimerfd()
{
    /// timerfd是基于文件描述符的定时器，通过文件描述符的可读事件进行超时通知
    /// CLOCK_REALTIME:相对时间，从1970.1.1到目前的时间。
    ///                更改系统时间会更改获取的值。它以系统时间为坐标。
    /// CLOCK_MONOTONIC:代表从过去某个固定的时间点开始的绝对的逝去时间
    ///                 获取的时间为系统重启到现在的时间，更改系统时间对其没有影响
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        LOG_FATAL("createTimerfd Failed timerfd=%d", timerfd);
    }
    return timerfd;
}

///
/// struct timeval
/// {
///     __time_t tv_sec;        /* Seconds. 秒*/
///     __suseconds_t tv_usec;  /* Microseconds. 微秒*/
/// };
/// int gettimeofday(struct timeval *tv, struct timezone *tz);
///
/// struct timespec
/// {
///     __time_t tv_sec;        /* Seconds. 秒*/
///     long int tv_nsec;       /* Nanoseconds. 纳秒*/
/// };
/// int clock_gettime(clockid_t clk_id, struct timespec *tp);
///
struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch()
                           - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = time_t(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = long((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    //LOG_TRACE("readTimerfd %d at %s", howmany, now.toString();
    if (n != sizeof howmany)
    {
        LOG_ERROR("readTimerfd reads %d bytes instead of 8", n);
    }
}

///
/// struct itimerspec{   
///     struct timespec it_interval; /*   timer period     */   
///     struct timespec it_value;    /*   timer expiration */   
/// };   
///
void resetTimerfd(int timerfd, Timestamp expiration)
{
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);

    /// timerfd:文件句柄
    /// 1代表绝对时间；为0代表相对时间
    /// newValue需要设置的时间
    /// oldValue这次设置之前的超时时间
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_ERROR("timerfd_settime fail. ret=%d", ret);
    }
}

}

TimerQueue::TimerQueue(EventLoop* loop)
    : FdHandler(loop, createTimerfd()),
      loop_(loop),
      timers_(),
      callingExpiredTimers_(false)
{
    enableReading();
    updateInPoll(AddEvent);
    
    muduo::LOG_INFO("TimerFd =%d in thread %d", fd_, loop->threadId());
}

TimerQueue::~TimerQueue()
{
    updateInPoll(DelEvent);
        
    sockets::close(fd_);

    for (TimerList::iterator it = timers_.begin();
         it != timers_.end(); ++it)
    {
        delete it->second;
    }
}

TimerId TimerQueue::addTimer(const TimerCallback& cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

TimerId TimerQueue::addTimer(TimerCallback&& cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if (earliestChanged)
    {
        resetTimerfd(fd_, timer->expiration());
    }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        delete it->first; // FIXME: no delete please
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead(Timestamp& receiveTime)
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(fd_, now);

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();

    for (std::vector<Entry>::iterator it = expired.begin();
    it != expired.end(); ++it)
    {
        it->second->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);
    
    for (std::vector<Entry>::iterator it = expired.begin();
        it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1); (void)n;
    }
    
    assert(timers_.size() == activeTimers_.size());
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;

    for (std::vector<Entry>::const_iterator it = expired.begin();
    it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        if (it->second->repeat()
            && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it->second->restart(now);
            insert(it->second);
        }
        else
        {
            // FIXME move to a free list
            delete it->second; // FIXME: no delete please
        }
    }

    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid())
    {
        resetTimerfd(fd_, nextExpire);
    }
}

bool TimerQueue::insert(Timer* timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    
    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second); (void)result;
    }
    
    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second); (void)result;
    }

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}

