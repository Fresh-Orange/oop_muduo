#ifndef _MUDUO_TCPCLIENT_H_
#define _MUDUO_TCPCLIENT_H_

#include "LinkOwner.h"
#include "EventLoop.h"
#include "ConnectSocket.h"

#include <string>

namespace muduo
{

class TcpClient : public LinkOwner
{
public:

    TcpClient(EventLoop* pMainLoop, const InetAddress& serverAddr,
              const std::string& nameArg);

    ~TcpClient();  

    void connect();

    void disconnect();

    void stop();

    void enableRetry() { retry_ = true; }

    bool retry() const { return retry_; }

    EventLoop* getLoop()  const override { return base_loop_; }

    void newConnection(int sockfd, const InetAddress& peerAddr);

    void delConnection(TcpLinkSPtr& conn);
    
    void rcvMessage(const TcpLinkSPtr& conn, Buffer* buf, Timestamp time);

    void writeComplete(const TcpLinkSPtr& conn);

    void highWaterMark(const TcpLinkSPtr& conn, size_t highMark);

    /// Not thread safe.
    void setConnectionCallback(NewConnectionCallback cb)
    { newConnectionCallback_ = std::move(cb); }

    /// Set message callback.
    /// Not thread safe.
    void setMessageCallback(RcvMessageCallback cb)
    { rcvMessageCallback_ = std::move(cb); }

    /// Set write complete callback.
    /// Not thread safe.
    void setWriteCompleteCallback(WriteCompleteCallback cb)
    { writeCompleteCallback_ = std::move(cb); }

private:

    NewConnectionCallback newConnectionCallback_; // 新建连接回调函数
    
    DelConnectionCallback delConnectionCallback_; // 关闭连接回调函数
    
    RcvMessageCallback    rcvMessageCallback_;    // 接收到新消息回调函数

    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成回调函数

    HighWaterMarkCallback highWaterMarkCallback_; //
    
private:

    EventLoop* base_loop_;     // TcpClient所在线程的EventLoop
    InetAddress serverAddr_;   // 需要连接的目标服务器地址
    const std::string name_;   // 客户端名字
    bool retry_;               // 客户端连接被动断开后是否需要持续尝试连接
    bool connect_;             // 记录客户端是否主动断开
    mutable MutexLock mutex_;
    ConnectSocketUPtr connectSocket_; // 客户端连接套接字
    int nextLinkId_;          // 每个连接需要不同的ID作为名字
    TcpLinkSPtr tcpLinkSPtr_;  // 记录连接对象的(智能)指针
};

}
#endif  // _MUDUO_TCPCLIENT_H_
