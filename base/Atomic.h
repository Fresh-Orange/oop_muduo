
#ifndef _MUDUO_ATOMIC_H_
#define _MUDUO_ATOMIC_H_

#include "copyable.h"
#include <stdint.h>

namespace muduo
{

/// gcc4.1.2�ṩ��__sync_*ϵ�е�built-in�����������ṩ�Ӽ����߼������ԭ�Ӳ�����
/// ��Щ������Ҫ����ʵ�����������
///
/// bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
/// type __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)
///
/// �����������ṩԭ�ӵıȽϺͽ��������*ptr == oldval,�ͽ�newvalд��*ptr,
/// ��һ����������Ȳ�д�������·���true,�ڶ��������ڷ��ز���֮ǰ��ֵ��
///
/// __sync_fetch_and_xxxϵ��һ����ʮ�����������м�/��/��/��/���/�Ⱥ�����ԭ�Ӳ���
/// 
/// __sync_fetch_and_add����˼�壬��fetch��Ȼ���Լӣ����ص����Լ���ǰ��ֵ��
/// __sync_add_and_fetch����˼�������Լ��ٷ��ء�������i++��++i�Ĺ�ϵ��һ���ġ�
/// 
/// �������������������ڶ��̶߳�ȫ�ֱ��������Լӣ�����Ҳ�������߳�����
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
        // ���value_ == 0,�ͽ�0д��value_,������value_֮ǰ��ֵ
        return __sync_val_compare_and_swap(&value_, 0, 0);
    }

    T getAndAdd(T x)
    {
        // �ȷ���value_��Ȼ��value_=value_+x
        return __sync_fetch_and_add(&value_, x); 
    }

    T addAndGet(T x)
    {
        // ��value_=value_+x��Ȼ�󷵻�value_
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
        // ��*ptr��ΪnewValue,������*ptr����֮ǰ��ֵ
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
        // ��*ptr��ΪnewValue,������*ptr����֮ǰ��ֵ
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

