//
// Created by thelx on 2020/6/22.
//
#ifndef OOP_MUDUO_IOMULTIPLEX_H
#define OOP_MUDUO_IOMULTIPLEX_H



#include <vector>
#include <map>
#include "FdHandler.h"
#include "../base/Timestamp.h"


namespace muduo{
    class EventLoop;

    class IOMultiplex{
    public:
        typedef std::vector<FdHandler*>  HandlerList;

        explicit IOMultiplex(EventLoop *ownerLoop)
        : ownerLoop_(ownerLoop){}

        virtual ~IOMultiplex()= default;

        virtual Timestamp poll(int timeoutMs, HandlerList* activeFdHandler) = 0;

        virtual void removeHandler(FdHandler* fdHandler) = 0;

        virtual void updateHandler(FdHandler* fdHandler) = 0;

        bool hasHandler(FdHandler* fdHandler){
            return fd2handler.find(fdHandler->fd()) != fd2handler.end();
        }

    protected:
        EventLoop* ownerLoop_;
        std::map<int, FdHandler*> fd2handler;
    };
}

#endif //OOP_MUDUO_IOMULTIPLEX_H