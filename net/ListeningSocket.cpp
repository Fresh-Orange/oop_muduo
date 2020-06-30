//
// Created by thelx on 2020/6/19.
//

#include "FdHandler.h"
#include "ListeningSocket.h"
#include "../base/Logging.h"
#include "../base/InetAddress.h"
#include "../base/Socket.h"
#include <unistd.h>

#include <utility>
#include <fcntl.h>
#include <cstring>


using namespace muduo;


ListeningSocket::ListeningSocket(EventLoop *loop,
        LinkOwner* owner,
        const InetAddress& listenAddr)
        : FdHandler(loop, sockets::createNonBlockSocket(listenAddr.family())),
        owner_(owner),
          idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)){

    setReuseAddr(true);
    setReusePort(true);
    bindAddr(listenAddr);
}

///
/// SO_REUSEADDR提供如下四个功能：
/// 1.一般一个端口释放后会等待两分钟之后才能再被使用
///   SO_REUSEADDR让端口释放后立即就可以被再次使用
///   这通常是重启监听服务器时有用，若不设置此选项，则bind时将出错。
/// 2.允许在同一端口上启动同一服务器的多个实例，只要每个实例捆绑一个不同的本地IP。
///   对于TCP，不可能启动捆绑相同IP和相同端口号的多个TCP服务器实例。
/// 3.允许单个进程捆绑同一端口到多个套接口上，只要每个捆绑指定不同的IP地址，
///   一般不用于TCP服务器。
/// 4.允许完全重复的捆绑：当一个IP地址和端口绑定到某个套接口上时，还允许此IP地址和端口捆绑到另一个套接口上。
///   这个特性仅在支持多播的系统上才有（TCP不支持多播,UDP支持）
///
void ListeningSocket::setReuseAddr(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, socklen_t(sizeof opt));
}

///
/// SO_REUSEPORT支持多个进程或者线程绑定到同一端口，提高服务器性能：
/// 允许多个套接字 bind()/listen() 同一个TCP/UDP端口
/// --- 每一个线程拥有自己的服务器套接字
/// --- 不存在多线程竞争互斥锁访问同一个服务器套接字的情况
/// 内核层面实现负载均衡
/// 安全层面，监听同一个端口的套接字只能位于同一个用户下面
///
/// 此特性在Linux kernel 3.9以后的版本开始支持
///
void ListeningSocket::setReusePort(bool on)
{
    int opt = on ? 1 : 0;
    int ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, socklen_t(sizeof opt));
    if (ret < 0 && on)
    {
        LOG_ERROR("REUSEPORT failed ret=%d");
    }
}

void ListeningSocket::startListen() {
    int ret = ::listen(fd_, SOMAXCONN);
    if (ret < 0){
        LOG_FATAL("listen error!");
    }
    else
        isListening_ = true;

    // muduo::LOG_INFO("ListenSocket fd=%d in thread %d", fd_, tcpServer_->getLoop()->threadId());
}

void ListeningSocket::handleRead(Timestamp& timestamp) {
    InetAddress peerAddr;

    int connfd = accept(&peerAddr);
    if (connfd >= 0)
    {
        owner_->newConnection(connfd, peerAddr);
    }
    else
    {   // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        if (errno == EMFILE)
        {
            ::close(idleFd_);
            idleFd_ = ::accept(fd_, NULL, NULL);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}

int ListeningSocket::accept(InetAddress* peeraddr)
{
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof addr);
    socklen_t addrlen = sizeof(struct sockaddr_in6);

    int connfd = ::accept(fd_, (struct sockaddr *)&addr, &addrlen);
    if (connfd < 0)
    {
        LOG_FATAL("accept error=%d", errno);
    }

    peeraddr->setSockAddrInet6(addr);

    // non-block
    int flags = ::fcntl(connfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    ::fcntl(connfd, F_SETFL, flags);

    // close-on-exec
    flags = ::fcntl(connfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ::fcntl(connfd, F_SETFD, flags);

    return connfd;
}

ListeningSocket::~ListeningSocket() {
    ::close(idleFd_);
}

void ListeningSocket::bindAddr(const InetAddress &addr) {
    int ret = ::bind(fd_, addr.getSockAddr(), static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (ret < 0)
    {
        LOG_FATAL("sockets::bindOrDie");
    }
}

int ListeningSocket::createNonBlockSocket(sa_family_t family)
{
    int sockfd = ::socket(family, SOCK_STREAM |
                                  SOCK_NONBLOCK |
                                  SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("createSocket fail, errno=%d");
    }
    return sockfd;
}

void ListeningSocket::handleWrite() {

}

void ListeningSocket::handleClose() {

}

void ListeningSocket::handleError() {

}
