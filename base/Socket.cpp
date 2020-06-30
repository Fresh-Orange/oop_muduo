
#include "Socket.h"
#include "Logging.h"
#include <string.h>
#include <unistd.h> // close


using namespace muduo;

struct sockaddr_in6 sockets::getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, (sockaddr*)(&localaddr), &addrlen) < 0)
    {
        LOG_FATAL("getsockname fial");
    }
    return localaddr;
}


struct sockaddr_in6 sockets::getPeerAddr(int sockfd)
{
  struct sockaddr_in6 peeraddr;
  bzero(&peeraddr, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (::getpeername(sockfd, (sockaddr*)(&peeraddr), &addrlen) < 0)
  {
    LOG_FATAL("getpeername fial");
  }
  return peeraddr;
}

int sockets::createNonBlockSocket(sa_family_t family)
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

int sockets::connect(int sockfd, const struct sockaddr* addr)
{
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

void sockets::close(int sockfd)
{
    if (::close(sockfd) < 0)
    {
        LOG_ERROR("close fd=%d fail");
    }
}

int sockets::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

bool sockets::isSelfConnect(int sockfd)
{
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin6_family == AF_INET)
    {
        const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
        const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
        return (laddr4->sin_port == raddr4->sin_port)
            && (laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr);
    }
    else if (localaddr.sin6_family == AF_INET6)
    {
        return (localaddr.sin6_port == peeraddr.sin6_port)
            && (memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr)) == 0;
    }
    else
    {
        return false;
    }
}

void sockets::shutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

