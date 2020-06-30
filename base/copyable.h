

#ifndef _MUDUO_COPYABLE_H_
#define _MUDUO_COPYABLE_H_

#include <memory>

namespace muduo
{

// 一个标签用于表示一个可拷贝的类，无实意
class copyable
{
};

// 禁止拷贝构造的类
class noncopyable
{
protected:
    noncopyable(){}
private:
    noncopyable(const noncopyable&);
    noncopyable& operator =(const noncopyable&);
};

// C++11缺少std::make_unique，C++14开始支持std::make_unique
// 在C++11中使用make_unique
template<typename T,typename ...Args>inline
std::unique_ptr<T> make_unique(Args&&...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

};

#endif  // _MUDUO_COPYABLE_H_


