#ifndef _MUDUO_HTTPSERVER_H_
#define _MUDUO_HTTPSERVER_H_

#include "../../base/copyable.h"
#include "../../net/TcpServer.h"

namespace muduo
{

class HttpRequest;
class HttpResponse;

class HttpServer : noncopyable, public TcpServer
{
public:
    HttpServer(EventLoop* pMainLoop,
               const InetAddress& listenAddr,
               const std::string& name,
               int threadnum);
               
    ~HttpServer();
    
    void start();  // calls server_.start();

    typedef std::function<void(const HttpRequest&, HttpResponse*)> HttpCallback;
    
    void setHttpCallback(const HttpCallback& cb)
    {
        httpCallback_ = cb;
    }


    void newConnection(int socketfd, const InetAddress &peerAddr) override;

    void writeComplete(const TcpLinkSPtr &conn) override;

    void highWaterMark(const TcpLinkSPtr &conn, size_t highMark) override;


    void delConnection(muduo::TcpLinkSPtr& conn) override ;

    void rcvMessage(const TcpLinkSPtr& conn,
                    Buffer* buf, Timestamp time) override;

    void rcvRequest(const TcpLinkSPtr& conn,
                    const HttpRequest& httpRequest);


private:

    HttpCallback httpCallback_;
};
}

#endif //_MUDUO_HTTPSERVER_H_
