//
// Created by thelx on 2020/6/22.
//

#include "Epoll.h"
#include <poll.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string>
#include <memory.h>
#include<string.h>
#include "../base/Logging.h"
using namespace muduo;


Epoll::Epoll(EventLoop *ownerLoop)
    : IOMultiplex(ownerLoop),
    events_(kInitEventListSize),
    epoll_fd(epoll_create1(EPOLL_CLOEXEC))
    {}

Epoll::~Epoll() {
    close(epoll_fd);
}

Timestamp Epoll::poll(int timeoutMs, IOMultiplex::HandlerList* activeFdHandlers) {
    // LOG_TRACE << "fd total count " << channels_.size();
    int numEvents = ::epoll_wait(epoll_fd,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_TRACE("%d events happened", numEvents);

        // fill active events
        for (int i = 0; i < numEvents; ++i)
        {
            auto* fdHandler = static_cast<FdHandler*>(events_[i].data.ptr);
            LOG_TRACE("poll-> events_[i].events = %d", events_[i].events);
            fdHandler->set_revents(events_[i].events);
            activeFdHandlers->push_back(fdHandler);
        }

        if (static_cast<size_t>(numEvents) == events_.size()) // 动态改变events数组大小
        {
            events_.resize(events_.size()*2);
        }
    }
    else if (numEvents < 0)
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR("EPollPoller::poll()");
        }
    }
    return now;
}

void Epoll::removeHandler(FdHandler *fdHandler) {
    int fd = fdHandler->fd();
    assert(fd2handler.find(fd) != fd2handler.end());
    assert(fd2handler[fd] == fdHandler);
    assert(fdHandler->isNoneEvent());

    fd2handler.erase(fd);

    if (fdHandler->getStateInPoll() == FdHandler::StateInPoll::Added)
    {
        update(EPOLL_CTL_DEL, fdHandler);
    }
    fdHandler->setStateInPoll(FdHandler::StateInPoll::New);
}

void Epoll::updateHandler(FdHandler *fdHandler) {
    FdHandler::StateInPoll state = fdHandler->getStateInPoll();
    int fd = fdHandler->fd();
    if (state == FdHandler::StateInPoll::New || state == FdHandler::StateInPoll::Deleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        if (state == FdHandler::StateInPoll::New)
        {
            assert(fd2handler.find(fd) == fd2handler.end());
            fd2handler[fd] = fdHandler;
        }

        fdHandler->setStateInPoll(FdHandler::StateInPoll::Added);
        update(EPOLL_CTL_ADD, fdHandler);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        assert(fd2handler.find(fd) != fd2handler.end());
        assert(fd2handler[fd] == fdHandler);
        assert(state == FdHandler::StateInPoll::Added);
        if (fdHandler->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, fdHandler);
            fdHandler->setStateInPoll(FdHandler::StateInPoll::Deleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, fdHandler);
        }
    }
}

void Epoll::update(int operation, FdHandler* fdHandler)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = fdHandler->events();
    event.data.ptr = fdHandler;

    int fd = fdHandler->fd();

    if (::epoll_ctl(epoll_fd, operation, fd, &event) < 0)
    {
        LOG_FATAL("epoll_ctl op = %s, fd = %d", operation2String(operation).c_str(), fd);
    }
}

std::string Epoll::operation2String(int op)
{
    switch (op)
    {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            return "Unknown Operation";
    }
}


