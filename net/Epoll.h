//
// Created by thelx on 2020/6/22.
//

#ifndef OOP_MUDUO_EPOLL_H
#define OOP_MUDUO_EPOLL_H


#include "IOMultiplex.h"
#include <sys/epoll.h>

namespace muduo{

    class EventLoop;

    class Epoll: public IOMultiplex{
    public:
        Epoll(EventLoop *ownerLoop);

        virtual ~Epoll();

        Timestamp poll(int timeoutMs, HandlerList* activeFdHandler) override;

        void removeHandler(FdHandler *fdHandler) override;

        void updateHandler(FdHandler *fdHandler) override;

    private:
        void update(int operation, FdHandler* fdHandler);
        std::string operation2String(int op);

        static const int kInitEventListSize = 16;
        typedef std::vector<epoll_event> EpollEventList;
        EpollEventList events_;
        int epoll_fd;

    };
}

#endif //OOP_MUDUO_EPOLL_H