//
// Created by thelx on 2020/6/26.
//

#include "LinkedSocket.h"
#include "LinkOwner.h"
#include "FdHandler.h"
#include "EventLoop.h"
#include "../base/Socket.h"
#include <unistd.h>
using namespace muduo;

LinkedSocket::LinkedSocket(LinkOwner* owner,
                 EventLoop* loop,
                 const std::string& name,
                 int sockfd,
                 const InetAddress& localAddr,
                 const InetAddress& peerAddr)
        : FdHandler(loop, sockfd),
          state_(kConnected),
          owner_(owner),
          name_(name),
          localAddr_(localAddr),
          peerAddr_(peerAddr),
          context_(NULL)
{

}

LinkedSocket::~LinkedSocket()
{
}

void LinkedSocket::send(const StringPiece& message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(std::bind(&bindSendInLoop,
                                             this,  message.as_string()));
        }
    }
}

void LinkedSocket::send(Buffer* buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            loop_->runInLoop(std::bind(&bindSendInLoop,
                                             this, buf->retrieveAllAsString()));
        }
    }
}

void LinkedSocket::bindSendInLoop(LinkedSocket* conn, const std::string& message)
{
    conn->sendInLoop(message.data(), message.size());
}

void LinkedSocket::sendInLoop(const StringPiece& message)
{
    sendInLoop(message.data(), message.size());
}

void LinkedSocket::sendInLoop(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected)
    {
        LOG_WARN("disconnected, give up writing");
        return;
    }
    // if no thing in output queue, try writing directly
    if (!isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(fd_, data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0)
            {
                owner_->writeComplete(shared_from_this());
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= len);
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_)
        {
            owner_->highWaterMark(shared_from_this(), oldLen + remaining);
        }
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        if (!isWriting())
        {
            enableWriting();
            updateInPoll(ModEvent);
        }
    }
}

void LinkedSocket::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&LinkedSocket::shutdownInLoop, this));
    }
}

void LinkedSocket::shutdownInLoop()
{
    loop_->assertInLoopThread();

    if (!isWriting())
    {
        LOG_TRACE("shutdownWrite fd = %d", fd_);
        sockets::shutdownWrite(fd_);
    }
}

void LinkedSocket::forceClose()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&LinkedSocket::forceCloseInLoop, shared_from_this()));
    }
}

void LinkedSocket::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        handleClose();
    }
}


void LinkedSocket::handleRead(Timestamp& receiveTime)
{
    LOG_TRACE("LinkedSocket reading");
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(fd_, &savedErrno);
    if (n > 0)
    {
        // 通知客户端或服务器接收新的消息
        owner_->rcvMessage(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        LOG_TRACE("handleRead == 0, closing");
        handleClose();
    }
    else
    {
        errno = savedErrno;
        handleError();
    }
}

void LinkedSocket::handleWrite()
{
    loop_->assertInLoopThread();
    if (isWriting())
    {
        ssize_t n = ::write(fd_, outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                disableWriting();
                updateInPoll(ModEvent);

                owner_->writeComplete(shared_from_this());

                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_TRACE("Connection fd=%d no more writing", fd_);
    }
}

void LinkedSocket::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE("LinkedSocket::handleClose, now state = %d", state_);
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);

    TcpLinkSPtr tcpLinkSPtr(shared_from_this());
    owner_->delConnection(tcpLinkSPtr);
}

void LinkedSocket::handleError()
{
    int err = sockets::getSocketError(fd_);
    LOG_ERROR("LinkedSocket handleError = %s", strerror(err));
}

void LinkedSocket::prepare() {
    LOG_TRACE("LinkedSocket::prepare, fd = %d", fd_);
    enableReading();
    updateInPoll(AddEvent);
}

