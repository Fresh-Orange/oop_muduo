
#ifndef _MUDUO_INETADDRESS_H_
#define _MUDUO_INETADDRESS_H_

#include <string>
#include <stdint.h>
#include <netinet/in.h>



namespace muduo
{

///
/// Wrapper of sockaddr_in.
///
/// This is an POD interface class.
class InetAddress 
{
public:
    /// Constructs an endpoint with given port number.
    /// Mostly used in TcpServer listening.
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

    /// Constructs an endpoint with given ip and port.
    /// @c ip should be "1.2.3.4"
    InetAddress(const char* ip, uint16_t port, bool ipv6 = false);

    /// Constructs an endpoint with given struct @c sockaddr_in
    /// Mostly used when accepting new connections
    explicit InetAddress(const struct sockaddr_in& addr)
    : addr_(addr)
    { }
    
    explicit InetAddress(const struct sockaddr_in6& addr)
    : addr6_(addr)
    { }

    sa_family_t family() const { return addr_.sin_family; }
    
    std::string toIpPort() const;
    
    std::string toIp() const;
    
    const struct sockaddr* getSockAddr() const
    {
        return (sockaddr*)(&addr6_);
    }
    
    void setSockAddrInet6(const struct sockaddr_in6& addr6) 
    { addr6_ = addr6; }
    
private:

    /*
    struct sockaddr {
        unsigned short sa_family; // address family, AF_xxx
        char sa_data[14];         // 14 bytes of protocol address 
    };
    
    struct sockaddr_in {
        sa_family_t sin_family;   //iPv4µØÖ·×å
        in_port_t sin_port;       //¶Ë¿ÚºÅ
        struct in_addr sin_addr;  //IPV4 address
        char sin_zero[8];
    };
    
    struct sockaddr_in6 {
        sa_family_t sin6_family;   // AF_INET6 (sa_family_t)
        in_port_t sin6_port;       // Transport layer port # (in_port_t)
        __uint32_t sin6_flowinfo;  // IP6 flow information
        struct in6_addr sin6_addr; // IP6 address
        __uint32_t sin6_scope_id;  // scope zone index
    };
    */

    union
    {
        struct sockaddr_in  addr_;
        struct sockaddr_in6 addr6_;
    };

};

}

#endif  // _MUDUO_INETADDRESS_H_

