
#include "TcpClient.h"
#include "EventLoop.h"
#include "../base/Socket.h"
#include "../base/copyable.h"

using namespace muduo;


TcpClient::TcpClient(EventLoop* pMainLoop, const InetAddress& serverAddr,
                     const std::string& nameArg)
    : base_loop_(pMainLoop),
      serverAddr_(serverAddr),
      name_(nameArg),
      retry_(false),
      connect_(true),
      nextLinkId_(1)
{
    connectSocket_ = make_unique<ConnectSocket>(this, base_loop_, serverAddr);
}

TcpClient::~TcpClient()
{
    // 需要考虑断开客户端连接
}

void TcpClient::connect()
{
    connect_ = true;
    connectSocket_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;
    
    {
        MutexLockGuard lock(mutex_);
        if (tcpLinkSPtr_)
        {
            tcpLinkSPtr_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connectSocket_->stop();
}

void TcpClient::newConnection(int sockfd, const InetAddress& peerAddr)
{
    InetAddress peerAddr1(sockets::getPeerAddr(sockfd));
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    
    char buf[32]={0};
    snprintf(buf, sizeof buf, "%s#%d", 
             peerAddr1.toIpPort().c_str(), nextLinkId_);
    ++nextLinkId_;
    std::string linkName(buf);
    
    LOG_INFO("thread=%d new link fd=%d name=%s from %s", 
             base_loop_->threadId(), sockfd, linkName.c_str(),
             peerAddr1.toIpPort().c_str());
              
    tcpLinkSPtr_ = std::make_shared<LinkedSocket>(this, base_loop_, linkName,
                                            sockfd, localAddr, peerAddr1);

    tcpLinkSPtr_->enableReading();
    tcpLinkSPtr_->updateInPoll(AddEvent);

    if (newConnectionCallback_)
    {
        newConnectionCallback_(tcpLinkSPtr_);
    }
}


void TcpClient::delConnection(TcpLinkSPtr& conn)
{
    LOG_INFO("thread=%d del link fd=%d name=%s from %s",
             base_loop_->threadId(), conn->fd(), conn->name().c_str(),
             conn->peerAddr().toIpPort().c_str());
             
    if (delConnectionCallback_)
    {
        delConnectionCallback_(tcpLinkSPtr_);
    }
    
    conn->updateInPoll(DelEvent);
    
    tcpLinkSPtr_.reset();

    if (retry_ && connect_)
    {
        connectSocket_->restart();
    }
}

void TcpClient::rcvMessage(const TcpLinkSPtr& conn, 
                           Buffer* buf, Timestamp time)
{
    if (rcvMessageCallback_)
    {
        rcvMessageCallback_(tcpLinkSPtr_, buf, time);
    }
}

void TcpClient::writeComplete(const TcpLinkSPtr& conn)
{
    if (writeCompleteCallback_)
    {
        writeCompleteCallback_(conn);
    }
}

void TcpClient::highWaterMark(const TcpLinkSPtr& conn, size_t highMark)
{
    if (highWaterMarkCallback_)
    {
        highWaterMarkCallback_(conn, highMark);
    }
}


