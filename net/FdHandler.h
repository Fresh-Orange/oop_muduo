//
// Created by thelx on 2020/6/18.
//

#ifndef OOP_MUDUO_FDHANDLER_H
#define OOP_MUDUO_FDHANDLER_H

#include <memory>
#include <poll.h>
#include "../base/Timestamp.h"
#include <assert.h>
#include "../base/Logging.h"
//#include "EventLoop.h"

namespace muduo{


    enum OptFlag{AddEvent, ModEvent, DelEvent};


    class EventLoop;

    class FdHandler{
    public:
        FdHandler(EventLoop* loop, int fd)
                :loop_(loop),
                 fd_(fd),
                 events_(0),
                 revents_(0),
                 state_in_poll(New)
        {}


        virtual ~FdHandler();

        void enableWriting();
        void enableReading();
        void disableWriting();
        void disableReading();
        void disableAll();

        // TODO: handleEvent需要是虚函数吗？
        void handleEvent(Timestamp timestamp);
        virtual void handleRead(Timestamp& timestamp) {};
        virtual void handleWrite(){};
        virtual void handleClose(){};
        virtual void handleError(){};

        bool isNoneEvent() const { return events_ == kNoneEvent; }
        bool isWriting()   const { return events_ &  kWriteEvent;}
        bool isReading()   const { return events_ &  kReadEvent; }

        int fd(){
            return fd_;
        }
        int events(){
            return events_;
        }
        void set_revents(int revents){
            LOG_TRACE("fd = %d, events_ = %d", fd_, events_);
            LOG_TRACE("revents_ = %d, revents = %d", revents_, revents);
            revents_ = revents;
        }
        std::string event2string() const;

        void updateInPoll(OptFlag optflag);

    public:
        enum StateInPoll {New, Added, Deleted};
        void setStateInPoll(StateInPoll state){
            state_in_poll = state;
        }
        StateInPoll getStateInPoll(){
            return state_in_poll;
        }

    protected:
        EventLoop* loop_; // 所属的eventloop
        int fd_; // 监控的文件描述符
        int events_;            // 用户设置期望监听的事件
        int revents_;           // Poller返回实际监听得到的事件

        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;
    private:
        StateInPoll state_in_poll;

    };

}

#endif //OOP_MUDUO_FDHANDLER_H
