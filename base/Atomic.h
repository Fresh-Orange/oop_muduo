
#ifndef _MUDUO_ATOMIC_H_
#define _MUDUO_ATOMIC_H_

#include "copyable.h"
#include <stdint.h>

namespace muduo
{

/// gcc4.1.2提供了__sync_*系列的built-in函数，用于提供加减和逻辑运算的原子操作。
/// 这些函数主要用于实现无锁化编程
///
/// bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
/// type __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)
///
/// 这两个函数提供原子的比较和交换，如果*ptr == oldval,就将newval写入*ptr,
/// 第一个函数在相等并写入的情况下返回true,第二个函数在返回操作之前的值。
///
/// __sync_fetch_and_xxx系列一共有十二个函数，有加/减/与/或/异或/等函数的原子操作
/// 
/// __sync_fetch_and_add顾名思义，先fetch，然后自加，返回的是自加以前的值。
/// __sync_add_and_fetch的意思就是先自加再返回。它们与i++和++i的关系是一样的。
/// 
/// 有了这两个函数，对于多线程对全局变量进行自加，就再也不用理线程锁了
///
template<typename T>
class AtomicIntegerT : noncopyable
{
public:
    AtomicIntegerT()
    : value_(0)
    {
    }
    
    AtomicIntegerT(T value)
    : value_(value)
    {
    }


    T get()
    {
        // 如果value_ == 0,就将0写入value_,并返回value_之前的值
        return __sync_val_compare_and_swap(&value_, 0, 0);
    }

    T getAndAdd(T x)
    {
        // 先返回value_，然后value_=value_+x
        return __sync_fetch_and_add(&value_, x); 
    }

    T addAndGet(T x)
    {
        // 先value_=value_+x，然后返回value_
        return __sync_add_and_fetch(&value_, x); 
    }

    T incrementAndGet()
    {
        return addAndGet(1);
    }

    T decrementAndGet()
    {
        return addAndGet(-1);
    }

    void add(T x)
    {
        getAndAdd(x);
    }

    void increment()
    {
        incrementAndGet();
    }

    void decrement()
    {
        decrementAndGet();
    }

    T getAndSet(T newValue)
    {
        // 将*ptr设为newValue,并返回*ptr操作之前的值
        return __sync_lock_test_and_set(&value_, newValue);
    }

    T compareAndSet(T compareValue, bool setValue)
    {
        return __sync_val_compare_and_swap(&value_, compareValue, setValue);
    }
private:
    volatile T value_;
};

class AtomicBool : noncopyable
{
public:

    AtomicBool(bool flag = true)
    : value_(flag)
    {
    }

    bool compareAndSet(bool compareFlag, bool setFalg)
    {
        return __sync_bool_compare_and_swap(&value_, compareFlag, setFalg);
    }

    bool getAndSet(bool newFlag)
    {
        // 将*ptr设为newValue,并返回*ptr操作之前的值
        return __sync_lock_test_and_set(&value_, newFlag);
    }
    
private:

    volatile bool value_;

};

typedef AtomicIntegerT<int8_t>  AtomicInt8;
typedef AtomicIntegerT<int16_t> AtomicInt16;
typedef AtomicIntegerT<int32_t> AtomicInt32;
typedef AtomicIntegerT<int64_t> AtomicInt64;
}

#endif  // _MUDUO_ATOMIC_H_

