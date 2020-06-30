
#include "InetAddress.h"
#include "Logging.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace muduo;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
    if (ipv6)
    {
        bzero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;  
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = htobe16(port);
    }
    else
    {
        bzero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? INADDR_LOOPBACK : INADDR_ANY;
        addr_.sin_addr.s_addr = htobe32(ip);
        addr_.sin_port = htobe16(port);
    }
}

InetAddress::InetAddress(const char* ip, uint16_t port, bool ipv6)
{
    if (ipv6)
    {
        bzero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = htobe16(port);
        if (::inet_pton(AF_INET6, ip, &addr6_.sin6_addr) <= 0)
        {
            muduo::LOG_FATAL("IPaddress Error");
        }
    }
    else
    {
        bzero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        addr_.sin_port = htobe16(port);
        if (::inet_pton(AF_INET, ip, &addr_.sin_addr) <= 0)
        {
            muduo::LOG_FATAL("IPaddress Error");
        }
    }
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = "";
    size_t size = sizeof(buf);
    
    uint16_t port;
    const struct sockaddr* addr = getSockAddr();
    
    if (addr->sa_family == AF_INET)
    {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in* addr4 = (sockaddr_in*)addr;
        port = be16toh(addr4->sin_port);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
    else if (addr->sa_family == AF_INET6)
    {
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6* addr6 = (sockaddr_in6*)addr;
        port = be16toh(addr6->sin6_port);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }

    size_t end = ::strlen(buf);
    assert(size > end);
    snprintf(buf+end, size-end, ":%u", port);

    return buf;
}

std::string InetAddress::toIp() const
{
    char buf[64] = "";
    size_t size = sizeof(buf);
    const struct sockaddr* addr = getSockAddr();
    
    if (addr->sa_family == AF_INET)
    {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in* addr4 = (sockaddr_in*)addr;
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
    else if (addr->sa_family == AF_INET6)
    {
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6* addr6 = (sockaddr_in6*)addr;
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
    
    return buf;
}

