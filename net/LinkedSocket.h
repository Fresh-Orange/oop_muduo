//
// Created by thelx on 2020/6/26.
//

#ifndef OOP_MUDUO_LINKEDSOCKET_H
#define OOP_MUDUO_LINKEDSOCKET_H

#include <memory>
#include "FdHandler.h"
#include "../base/InetAddress.h"
#include "../base/Buffer.h"
#include "../base/Timestamp.h"

namespace muduo{
    class LinkedSocket;
    class LinkOwner;

    typedef std::shared_ptr<LinkedSocket> TcpLinkSPtr;
    typedef std::function<void (const TcpLinkSPtr&)> NewConnectionCallback;
    typedef std::function<void (const TcpLinkSPtr&)> DelConnectionCallback;
    typedef std::function<void (const TcpLinkSPtr&, Buffer*, Timestamp)> RcvMessageCallback;
    typedef std::function<void (const TcpLinkSPtr&)> WriteCompleteCallback;
    typedef std::function<void (const TcpLinkSPtr&, size_t)> HighWaterMarkCallback;

    class LinkedSocket : public FdHandler,
                    public std::enable_shared_from_this<LinkedSocket>
    {
    public:
        LinkedSocket(LinkOwner* owner,
                EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);

        virtual ~LinkedSocket();

        void handleRead(Timestamp& receiveTime) override ;

        void handleWrite() override;

        void handleClose();

        void handleError();

        void prepare();

        void send(Buffer* buf);

        void send(const StringPiece& message);

        void sendInLoop(const StringPiece& message);

        void sendInLoop(const void* data, size_t len);

        static void bindSendInLoop(LinkedSocket* conn, const std::string& message);

        void shutdown();

        void shutdownInLoop();

        void forceClose();

        void forceCloseInLoop();

        bool connected() const { return state_ == kConnected; }

        bool disconnected() const { return state_ == kDisconnected; }

        const std::string& name() const { return name_; }

        const InetAddress& peerAddr() const { return peerAddr_; }

        void setContext(void* context) { context_ = context; }

        void* getContext() { return context_; }

    private:

        enum StateE { kDisconnected, kConnected, kDisconnecting };

        void setState(StateE s) { state_ = s; }

        StateE state_;

        LinkOwner* owner_;

        const std::string name_;

        const InetAddress localAddr_;

        const InetAddress peerAddr_;

        size_t highWaterMark_;

        Buffer inputBuffer_;

        Buffer outputBuffer_;

        void* context_;
    };

}



#endif //OOP_MUDUO_LINKEDSOCKET_H
