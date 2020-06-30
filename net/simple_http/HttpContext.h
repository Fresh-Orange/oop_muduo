#ifndef _MUDUO_HTTPCONTEXT_H_
#define _MUDUO_HTTPCONTEXT_H_

#include "../../base/copyable.h"
#include "HttpRequest.h"
#include "../../base/Timestamp.h"


namespace muduo
{
class Buffer;

class HttpContext : public copyable
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };
    
    HttpContext()
    : state_(kExpectRequestLine)
    {
    }

    bool parseRequest(Buffer* buf, Timestamp receiveTime);

    bool gotAll() const { return state_ == kGotAll; }

    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const { return request_; }

    HttpRequest& request() { return request_; }
    
private:

    bool processRequestLine(const char* begin, const char* end);
    
    HttpRequestParseState state_;
    
    HttpRequest request_;
    
};
}
#endif //_MUDUO_HTTPCONTEXT_H_
