//
// Created by thelx on 2020/6/26.
//

#ifndef OOP_MUDUO_TCPSERVER_H
#define OOP_MUDUO_TCPSERVER_H
#include "LinkOwner.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "LinkedSocket.h"
#include "ListeningSocket.h"
#include "../base/InetAddress.h"

namespace muduo{

    typedef std::shared_ptr<LinkedSocket> TcpLinkSPtr;

    typedef std::function<void (const TcpLinkSPtr&)> ConnectionCallback;
    typedef std::function<void (const TcpLinkSPtr&)> CloseCallback;
    typedef std::function<void (const TcpLinkSPtr&)> WriteCompleteCallback;
    typedef std::function<void (const TcpLinkSPtr&, size_t)> HighWaterMarkCallback;
    typedef std::function<void (const TcpLinkSPtr&, Buffer*, Timestamp)> MessageCallback;

    class TcpServer : public LinkOwner{
    public:
        TcpServer(EventLoop *baseLoop, const InetAddress& listenAddr
                , const std::string &name, int threadNum);

        virtual ~TcpServer();

        void start();


        void setConnectionCallback(const ConnectionCallback& cb)
        { connectionCallback_ = cb; }

        void setMessageCallback(const MessageCallback& cb)
        { messageCallback_ = cb; }

        void setWriteCompleteCallback(const WriteCompleteCallback& cb)
        { writeCompleteCallback_ = cb; }

        void setHighWaterMarkCallback(const HighWaterMarkCallback &highWaterMarkCallback) {
            highWaterMarkCallback_ = highWaterMarkCallback;
        }

        virtual void newConnection(int socketfd, const InetAddress &peerAddr) override;

        virtual void delConnection(TcpLinkSPtr &conn) override;

        virtual void rcvMessage(const TcpLinkSPtr &conn, Buffer *buf, Timestamp time) override;

        virtual void writeComplete(const TcpLinkSPtr &conn) override;

        virtual void highWaterMark(const TcpLinkSPtr &conn, size_t highMark) override;

        EventLoop *getLoop() const override;


    protected:
        EventLoop* base_loop_;
        std::shared_ptr<EventLoopThreadPool> threadPoll_;
        std::unique_ptr<ListeningSocket> listeningSocket_;
        typedef std::map<std::string, TcpLinkSPtr> ConnectionMap;
        ConnectionMap name2connection;
        std::string name_;
        InetAddress listenAddr_;
        int connId_;
        ConnectionCallback connectionCallback_ = nullptr;
        CloseCallback closeCallback_ = nullptr;
        WriteCompleteCallback writeCompleteCallback_ = nullptr;
        HighWaterMarkCallback highWaterMarkCallback_ = nullptr;
        MessageCallback messageCallback_ = nullptr;


        AtomicInt32 started_;

        void removeConnectionInLoop(const TcpLinkSPtr &conn);
    };

}
#endif //OOP_MUDUO_TCPSERVER_H
