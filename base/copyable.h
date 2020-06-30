

#ifndef _MUDUO_COPYABLE_H_
#define _MUDUO_COPYABLE_H_

#include <memory>

namespace muduo
{

// һ����ǩ���ڱ�ʾһ���ɿ������࣬��ʵ��
class copyable
{
};

// ��ֹ�����������
class noncopyable
{
protected:
    noncopyable(){}
private:
    noncopyable(const noncopyable&);
    noncopyable& operator =(const noncopyable&);
};

// C++11ȱ��std::make_unique��C++14��ʼ֧��std::make_unique
// ��C++11��ʹ��make_unique
template<typename T,typename ...Args>inline
std::unique_ptr<T> make_unique(Args&&...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

};

#endif  // _MUDUO_COPYABLE_H_


