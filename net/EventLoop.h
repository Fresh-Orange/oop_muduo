
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
/// ���÷���������SIGPIPE�ź�
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
/// EventLoop�߳�һ�㱻������poll����
/// Ϊ���ܹ������̣߳�ʹ��eventfd���ƶ����¼�������
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
    /// ����EventLoop�߳�
    ///
    void wakeup();

    ///
    /// runInLoopΪ�û��ṩ��һ��������ע��EventLoop��ִ�еĽӿ�
    /// ��������߳̾���EventLoop��ִ���̣߳���Functor����ִ��
    /// ����Functor����������У��ȴ��̱߳�wakeup�Ժ����Ŷ�ִ��
    /// 
    void runInLoop(const Functor& cb);
    
    void queueInLoop(const Functor& cb);

    void doPendingFunctors();
    
    ///
    /// �ж�EventLoop�Ƿ����ڵ�ǰ�߳�
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
    /// ��ѯ���޸�������
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

    std::unique_ptr<IOMultiplex> poller_;      // ʹ������ָ���Զ�����Poller����

    std::unique_ptr<TimerQueue> timerQueue_;

    std::unique_ptr<WakeupFd> wakeupFd_;  // ͨ��eventfd�����߳�

    /// ���ڼ���״̬����������
    typedef std::vector<FdHandler*> FdEventList;
    FdEventList actFdEventList_;

    /// pendingFunctors_�����ⲿ�߳�ע��Ļص�����,��Ҫmutex����
    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;

    IgnoreSigPipe ignoreSigPipe;
};




typedef std::shared_ptr<EventLoop> EventLoopShaPtr;
typedef std::unique_ptr<EventLoop> EventLoopUniPtr;


}
#endif  // _MUDUO_EVENTLOOP_H_

