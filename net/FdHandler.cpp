//
// Created by thelx on 2020/6/19.
//
#include <memory>
#include <poll.h>
#include <sstream>
#include "FdHandler.h"
#include "EventLoop.h"
#include "../base/Logging.h"
#include "../base/Socket.h"

using namespace muduo;

const int FdHandler::kNoneEvent = 0;
const int FdHandler::kReadEvent = POLLIN | POLLPRI;
const int FdHandler::kWriteEvent = POLLOUT;

void FdHandler::handleEvent(Timestamp receiveTime) {

    ///
    /// POLLIN     0x0001 普通或优先级带数据可读
    /// POLLRDNORM 0x0040 普通数据可读
    /// POLLRDBAND 0x0080 优先级带数据可读
    /// POLLPRI    0x0002 高优先级数据可读
    ///
    /// POLLOUT    0x0004 可以不阻塞的写普通数据和优先级数据
    /// POLLWRNORM 0x0100 可以不阻塞的写普通数据
    /// POLLWRBAND 0x0200 可以不阻塞的写优先级带数据
    ///
    /// POLLERR    0x0008 发生错误
    /// POLLHUP    0x0010 管道的写端被关闭，读端描述符上接收到这个事件
    /// POLLNVAL   0x0020 描述符不是一个打开的文件
    ///
    /// POLLRDHUP  0x2000 TCP连接被对方关闭，或者对方关闭了写操作
    ///            客户端调用close()正常断开连接，在服务器端会触发一个
    ///            事件。在低于 2.6.17 版本的内核中，这个事件EPOLLIN即0x1
    ///            代表连接可读。然后服务器上层软件read连接，只能读到 EOF
    ///            2.6.17 以后的版本增加了EPOLLRDHUP事件，对端连接断开触发
    ///            的事件会包含 EPOLLIN | EPOLLRDHUP，即 0x2001
    ///
    if (revents_ & (POLLERR | POLLNVAL)){
        // 这里只记录错误，是否close
        // 需要在read流程确认实际的错误
        handleError();
    }

    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)){
        handleClose();
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)){
        handleRead(receiveTime);
    }

    if (revents_ & POLLOUT){
        handleWrite();
    }
}

void FdHandler::enableWriting() {
    events_ |= kWriteEvent;
    // todo: update in poller
}

void FdHandler::enableReading() {
    events_ |= kReadEvent;
    // todo: update in poller
}
void FdHandler::disableWriting() {
    events_ &= ~kWriteEvent;
    // todo: update in poller
}

void FdHandler::disableReading() {
    events_ &= ~kReadEvent;
    // todo: update in poller
}
void FdHandler::disableAll() {
    events_ = kNoneEvent;
    // todo: update in poller
}

FdHandler::~FdHandler() {
    disableAll();
    updateInPoll(DelEvent);
    sockets::close(fd_);
}

std::string FdHandler::event2string() const {
    std::ostringstream oss;
    oss << fd_ << ": ";
    if (events_ & POLLIN)
        oss << "IN ";
    if (events_ & POLLPRI)
        oss << "PRI ";
    if (events_ & POLLOUT)
        oss << "OUT ";
    if (events_ & POLLHUP)
        oss << "HUP ";
    if (events_ & POLLRDHUP)
        oss << "RDHUP ";
    if (events_ & POLLERR)
        oss << "ERR ";
    if (events_ & POLLNVAL)
        oss << "NVAL ";

    return oss.str();
}

void FdHandler::updateInPoll(OptFlag optflag)
{
    std::unique_ptr<IOMultiplex>& poller = loop_->getPoller();
    assert(poller.get() != NULL);

    switch(optflag)
    {
        case AddEvent:
            assert(getStateInPoll() != StateInPoll::Added);
            poller->updateHandler(this);
            return;

        case ModEvent:
            assert(getStateInPoll() == StateInPoll::Added);
            poller->updateHandler(this);
            return;

        case DelEvent:
            poller->removeHandler(this);
            return;
    }
}

