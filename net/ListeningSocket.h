//
// Created by thelx on 2020/6/19.
//

#ifndef OOP_MUDUO_LISTENINGSOCKET_H
#define OOP_MUDUO_LISTENINGSOCKET_H

#include "FdHandler.h"
#include "../base/InetAddress.h"
#include "LinkOwner.h"

namespace muduo{
    typedef std::function<void(int, const InetAddress&)> OnConnectFunc;
    class ListeningSocket: public FdHandler{
    public:

        ListeningSocket(EventLoop *loop, LinkOwner* owner, const InetAddress& listenAddr);
        virtual ~ListeningSocket();

        void handleRead(Timestamp& timestamp) override;

        void handleWrite() override;

        void handleClose() override;

        void handleError() override;

        void startListen();
        bool isListening(){
            return isListening_;
        }

    private:

        LinkOwner* owner_;
        int accept(InetAddress *peeraddr);
        int idleFd_;

        void bindAddr(const InetAddress& addr);
        void setReusePort(bool on);
        void setReuseAddr(bool on);
        bool isListening_ = false;

        static int createNonBlockSocket(sa_family_t family);
    };
}

#endif //OOP_MUDUO_LISTENINGSOCKET_H
