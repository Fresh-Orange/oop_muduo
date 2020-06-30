
#include "EventLoop.h"
#include "../base/Socket.h"
#include "FdHandler.h"
#include "../base/InetAddress.h"
#include "Epoll.h"
#include "TimerQueue.h"
#include "../base/Timer.h"

#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <algorithm>


using namespace muduo;

namespace muduo
{

static __thread EventLoop* t_loopInThread = 0;

const int kPollTimeMs = 10000;

}



EventLoop::EventLoop()
    :looping_(false), 
     quit_(false),
     functorPending_(false),
     pollCounter_(0),
     threadId_(CurrentThread::tid()),
     poller_(new Epoll(this)),
     timerQueue_(new TimerQueue(this)),
     wakeupFd_(new WakeupFd(this)),
     ignoreSigPipe(IgnoreSigPipe())
{
    if (t_loopInThread)
    {
        LOG_FATAL("Another EventLoop %x exists in this thread %d",
                  t_loopInThread, threadId_);
    }
    else
    {
        t_loopInThread = this;
        LOG_INFO("EventLoop created 0x%x in thread %d.", this, threadId_);
    }

    // 使能用于唤醒线程的描述符，并添加到poller监控描述符集合
    wakeupFd_->enableReading();
    wakeupFd_->updateInPoll(AddEvent);
}

EventLoop::~EventLoop()
{
    LOG_INFO("EventLoop 0x%x of thread %d destructs in thread %d", 
             this, threadId_, CurrentThread::tid());

    t_loopInThread = NULL;
}

void EventLoop::loop()
{
    assertInLoopThread();
    
    if (looping_)
    {
        return;
    }
    
    looping_ = true;
    quit_ = false;
    LOG_TRACE("EventLoop %x start looping", this);
    
    while (!quit_)
    {
        LOG_TRACE("EventLoop %x a new loop", this);
        LOG_TRACE("wakefd = %d is reading = %d", wakeupFd_->fd(), wakeupFd_->isReading());
        LOG_TRACE("poller has wakefd = %d", poller_->hasHandler(wakeupFd_.get()));
        actFdEventList_.clear();
        pollRetTime_ = poller_->poll(kPollTimeMs, &actFdEventList_);
        ++pollCounter_;
        
        /// 处理当前处于活动状态的描述符事件
        for (FdEventList::iterator it = actFdEventList_.begin();
             it != actFdEventList_.end(); ++it)
        {
            FdHandler* pFdEvent = *it;
            pFdEvent->handleEvent(pollRetTime_);
        }

        /// 执行外部线程注入的回调函数队列
        doPendingFunctors();
    }
     
    LOG_TRACE("EventLoop %x stop looping", this);
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    
    // 每个线程的EventLoop对象在线程栈上创建创建
    // 只要线程不结束，EventLoop对象总是有效的
    if (!isInLoopThread())
    {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}

void EventLoop::wakeup()
{
    wakeupFd_->wakeupWrite();
}

void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(const Functor& cb)
{
    LOG_TRACE("EventLoop::queueInLoop, Add an task");
    {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.push_back(cb);
    }

    if (!isInLoopThread() || functorPending_)
    {
        wakeupFd_->wakeupWrite();
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    functorPending_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }
    
    functorPending_ = false;
}

WakeupFd::WakeupFd(EventLoop* loop)
    : FdHandler(loop, createEventFd())
{
    muduo::LOG_INFO("WakeupFd fd=%d in thread %d", fd_, loop->threadId());
}

WakeupFd::~WakeupFd()
{
    // 基类析构即可
}

void WakeupFd::wakeupWrite()
{
    //MutexLockGuard lock(mutex_);

    LOG_TRACE("wakeupWrite, try write fd = %d which is reading = %d", fd_, isReading());
    uint64_t one = 1;
    ssize_t n = ::write(fd_, &one, sizeof one);
    LOG_TRACE("END wakeupWrite, try write fd = %d", fd_);

    if (n != sizeof one || errno == EAGAIN)
    {
        LOG_ERROR("Fail! write %d bytes", n);
    }
}

void WakeupFd::handleRead(Timestamp& receiveTime)
{
    //MutexLockGuard lock(mutex_);

    LOG_TRACE("wakeupRead, reading fd = %d", fd_);
    uint64_t one = 1;
    ssize_t n = ::read(fd_, &one, sizeof one);

    // usleep(100); // todo : just try

    printf("END wakeupRead, try read fd = %d\n", fd_);
    if (n != sizeof one || errno == EAGAIN)
    {
        printf("ERROR wakeupRead, try read fd = %d\n", fd_);
        LOG_ERROR("Fail! read %d bytes", n);
    }
}

int WakeupFd::createEventFd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("createEventfd Failed! evtfd=%d",evtfd);
    }
    return evtfd;
}



