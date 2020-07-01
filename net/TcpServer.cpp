//
// Created by thelx on 2020/6/26.
//

#include "TcpServer.h"
#include "../base/Socket.h"
#include <functional>
#include "../base/copyable.h"

using namespace muduo;

TcpServer::TcpServer(EventLoop *baseLoop, const InetAddress& listenAddr,
        const std::string &name, int threadNum)
:   LinkOwner(),
    base_loop_(baseLoop),
    name_(name),
    listenAddr_(listenAddr),
    connId_(1),
    threadPoll_(new EventLoopThreadPool(base_loop_, threadNum, name)) // todo: 这里的name
    // listeningSocket_(new ListeningSocket(base_loop_, listenAddr))
{
    listeningSocket_ = make_unique<ListeningSocket>(base_loop_, this, listenAddr);
}

TcpServer::~TcpServer() {
    base_loop_->assertInLoopThread();
    LOG_TRACE("TcpServer::~TcpServer [%s] destructing", name_.c_str());
}

void TcpServer::start()
{
    if (started_.getAndSet(1) == 0)
    {
        threadPoll_->start(NULL); // todo: 暂时设置位NULL
        assert(!listeningSocket_->isListening());
        base_loop_->runInLoop(
                std::bind(&ListeningSocket::startListen, listeningSocket_.get()));
        listeningSocket_->enableReading();
        listeningSocket_->updateInPoll(AddEvent);
    }
}


void TcpServer::newConnection(int socketfd, const InetAddress &peerAddr) {
    EventLoop* ioLoop_ptr = threadPoll_->getNextLoop();
    std::string ipPort = listenAddr_.toIpPort();
    char logBuf[64];
    snprintf(logBuf, 64, "ipPort: %s, connect ID: %d", ipPort.c_str(), connId_);
    connId_++;
    LOG_INFO("TcpServer::newConnection: name = %s, %s", name_.c_str(), logBuf);
    std::string connName = name_ + logBuf;

    InetAddress localAddr(sockets::getLocalAddr(socketfd));
    TcpLinkSPtr conn(new LinkedSocket(this, ioLoop_ptr, connName, socketfd, localAddr, peerAddr));

    name2connection[connName] = conn;
    ioLoop_ptr->runInLoop(std::bind(&LinkedSocket::prepare, conn));
}

void TcpServer::delConnection(TcpLinkSPtr &conn) {
    // todo: 注释掉runInLoop是因为无法通过eventfd唤醒主线程，原因未知
    // base_loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
    name2connection.erase(conn->name());
}

void TcpServer::removeConnectionInLoop(const TcpLinkSPtr & conn)
{
    base_loop_->assertInLoopThread();
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s", name_.c_str(), conn->name().c_str());
    name2connection.erase(conn->name());
}

void TcpServer::rcvMessage(const TcpLinkSPtr &conn, Buffer *buf, Timestamp time) {
    if (messageCallback_)
    {
        messageCallback_(conn, buf, time);
    }
}

void TcpServer::writeComplete(const TcpLinkSPtr &conn) {
    if (writeCompleteCallback_)
    {
        writeCompleteCallback_(conn);
    }
}

void TcpServer::highWaterMark(const TcpLinkSPtr &conn, size_t highMark) {
    if (highWaterMarkCallback_)
    {
        highWaterMarkCallback_(conn, highMark);
    }
}

EventLoop *TcpServer::getLoop() const {
    return base_loop_;
}








