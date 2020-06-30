
#ifndef _MUDUO_EVENTLOOP_H_
#define _MUDUO_EVENTLOOP_H_

#include "../base/copyable.h"
#include "FdHandler.h"
#include "../base/Logging.h"
#include "IOMultiplex.h"
#include "LinkedSocket.h"
#include "../base/ThreadUtil.h"
#include "../base/Timer.h"

#include <functional>
#include <memory>
#include <signal.h>


namespace muduo
{
class InetAddress;

///
/// 设置服务器忽略SIGPIPE信号
///
    class IgnoreSigPipe
    {
    public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
            LOG_TRACE("Ignore SIGPIPE");
        }
    };

///
/// EventLoop线程一般被阻塞在poll函数
/// 为了能够唤醒线程，使用eventfd机制定义事件描述符
///
    class WakeupFd : public FdHandler
    {
    public:
        WakeupFd(EventLoop* loop);

        virtual ~WakeupFd();

        void wakeupWrite();

        virtual void handleRead(Timestamp& receiveTime) override;

    private:
        int createEventFd();
        mutable MutexLock mutex_;
    };

///
/// Reactor, at most one per thread.
///
class EventLoop : noncopyable
{
public:
    typedef std::function<void()> Functor;
    
    EventLoop();
    
    virtual ~EventLoop();  // force out-line dtor, for scoped_ptr members.
    
    ///
    /// Loops forever.
    ///
    /// Must be called in the same thread as creation of the object.
    ///
    void loop();

    /// Quits loop.
    ///
    /// This is not 100% thread safe, if you call through a raw pointer,
    /// better to call through shared_ptr<EventLoop> for 100% safety.
    void quit();
    
    ///
    /// Runs callback at 'time'.
    /// Safe to call from other threads.
    ///
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    ///
    /// Runs callback after @c delay seconds.
    /// Safe to call from other threads.
    ///
    TimerId runAfter(double delay, const TimerCallback& cb);
    ///
    /// Runs callback every @c interval seconds.
    /// Safe to call from other threads.
    ///
    TimerId runEvery(double interval, const TimerCallback& cb);
    ///
    /// Cancels the timer.
    /// Safe to call from other threads.
    ///
    void cancel(TimerId timerId);

    ///
    /// 唤醒EventLoop线程
    ///
    void wakeup();

    ///
    /// runInLoop为用户提供了一个将函数注入EventLoop中执行的接口
    /// 如果调用线程就是EventLoop的执行线程，则Functor立刻执行
    /// 否则，Functor被放入队列中，等待线程被wakeup以后再排队执行
    /// 
    void runInLoop(const Functor& cb);
    
    void queueInLoop(const Functor& cb);

    void doPendingFunctors();
    
    ///
    /// 判断EventLoop是否属于当前线程
    ///
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            LOG_FATAL("EventLoop 0x%x was created in thread=%d,CurThread=%d",
                      this, threadId_, CurrentThread::tid());
        }
    }
    
    bool isInLoopThread() const 
    { 
        return threadId_ == muduo::CurrentThread::tid(); 
    }
    
    ///
    /// 查询、修改类属性
    ///
    Timestamp pollReturnTime() const { return pollRetTime_; }
    
    int64_t pollCounter() const { return pollCounter_; }

    std::unique_ptr<IOMultiplex>& getPoller() { return poller_; }

    pid_t threadId(){ return threadId_; }
    
protected:

    bool looping_;             // 
    bool quit_;                // 
    bool functorPending_;      // 
    
    int64_t pollCounter_;
    const pid_t threadId_;
    Timestamp pollRetTime_;

    std::unique_ptr<IOMultiplex> poller_;      // 使用智能指针自动管理Poller对象

    std::unique_ptr<TimerQueue> timerQueue_;

    std::unique_ptr<WakeupFd> wakeupFd_;  // 通过eventfd唤醒线程

    /// 处于激活状态描述符集合
    typedef std::vector<FdHandler*> FdEventList;
    FdEventList actFdEventList_;

    /// pendingFunctors_保存外部线程注入的回调函数,需要mutex保护
    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;

    IgnoreSigPipe ignoreSigPipe;
};




typedef std::shared_ptr<EventLoop> EventLoopShaPtr;
typedef std::unique_ptr<EventLoop> EventLoopUniPtr;


}
#endif  // _MUDUO_EVENTLOOP_H_

