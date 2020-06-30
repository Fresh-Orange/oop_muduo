#include "copyable.h"

#include <assert.h>
#include <pthread.h>
#include <deque>
#include <sys/types.h>
#include <sys/syscall.h>  
#include <sys/types.h> 


#ifndef _MUDUO_THREAD_UTIL_H_
#define _MUDUO_THREAD_UTIL_H_

using namespace std;

namespace muduo
{

    namespace CurrentThread
    {
        // internal
        extern __thread int t_cachedTid;
        extern __thread char t_tidString[32];
        extern __thread int t_tidStringLength;
        extern __thread const char* t_threadName;
        void cacheTid();

        inline int tid()
        {
            if (__builtin_expect(t_cachedTid == 0, 0))
            {
                cacheTid();
            }
            return t_cachedTid;
        }

        inline const char* tidString() // for logging
        {
            return t_tidString;
        }

        inline int tidStringLength() // for logging
        {
            return t_tidStringLength;
        }

        inline const char* name()
        {
            return t_threadName;
        }

        bool isMainThread();

        void sleepUsec(int64_t usec);  // for testing

        string stackTrace(bool demangle);
    }  // namespace CurrentThread

class MutexLock : public noncopyable
{
public:
    MutexLock()
    : holder_(0)
    {
        pthread_mutex_init(&mutex_, NULL);
    }

    ~MutexLock()
    {
        assert(holder_ == 0);
        pthread_mutex_destroy(&mutex_);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex_);
        assignHolder();
    }

    void unlock()
    {
        unassignHolder();
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t* getMutex() /* non-const */
    {
        return &mutex_;
    }
private:
    void unassignHolder()
    {
        holder_ = 0;
    }

    void assignHolder()
    {
        //holder_ = CurrentThread::tid();
    }
  
    pthread_mutex_t mutex_;
    pid_t holder_;
};

class MutexLockGuard : noncopyable
{
public:
    explicit MutexLockGuard(MutexLock& mutex)
    : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~MutexLockGuard()
    {
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;
};

// ���ݶ��̱߳�̾���:��������һ���뻥����һ��ʹ��
// Condition�����MutexLocking����mutex���������ǵĳ�Ա��������MutexLock&
class Condition : noncopyable
{
public:
    explicit Condition(MutexLock& mutex)
    : mutex_(mutex)
    {
        pthread_cond_init(&pcond_, NULL);
    }

    ~Condition()
    {
        pthread_cond_destroy(&pcond_);
    }

    void wait()
    {
        pthread_cond_wait(&pcond_, mutex_.getMutex());
    }

    // ��ʱ����true, ��������false���˺���Ӧ�ú�����
    bool waitForSeconds(double seconds);

    void notify() // ֪ͨ����һ���������������߳�,�˺���Ӧ�ú�����
    {
        pthread_cond_signal(&pcond_);
    }

    void notifyAll() // ֪ͨ���б������������߳�
    {
        pthread_cond_broadcast(&pcond_);
    }

private:
    MutexLock& mutex_;
    pthread_cond_t pcond_;
};

// ����ʱ�����߳�ͬ������������
// 1.���̵߳ȴ�������̸߳������һ����������߳���ִ��
// 2.���߳����һ�������֪ͨ�������߳�ִ��
// 
class CountDownLatch : noncopyable
{
public:
    explicit CountDownLatch(int count)
    : count_(count), mutex_(), condition_(mutex_) // ����ĳ�ʼ��˳�����Ҫ
    {
    }
    
    void wait();
    void countDown();
    int getCount() const;

private:
    int count_;
    mutable MutexLock mutex_;
    Condition condition_;
    
};

// ����һ��ģ���࣬����ʵ�ִ����������ܵĴ洢����
// �ڲ���ԭ����һ��˫�ڶ���+������+��������
template<typename T>
class BlockingQueue : noncopyable
{
 public:
    BlockingQueue()
    : mutex_(), notEmpty_(mutex_), queue_()
    {
    }

    void put(const T& x)
    {
        MutexLockGuard lock(mutex_);
        queue_.push_back(x);
        notEmpty_.notify(); // wait morphing saves us
    }

    void put(T&& x)
    {
        MutexLockGuard lock(mutex_);
        queue_.push_back(std::move(x));
        notEmpty_.notify();
    }

    T take()
    {
        MutexLockGuard lock(mutex_);
        // always use a while-loop, due to spurious wakeup
        while (queue_.empty())
        {
            notEmpty_.wait();
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        return front;
    }

    size_t size() const
    {
        MutexLockGuard lock(mutex_);
        return queue_.size();
    }

private:
    mutable MutexLock mutex_;
    Condition         notEmpty_;
    std::deque<T>     queue_;
};

}

// ��ֱֹ�ӵ��ù��캯��������ʱ��
#define MutexLocking(x)    error "Missing guard object name"

#endif  // _MUDUO_THREAD_UTIL_H_

