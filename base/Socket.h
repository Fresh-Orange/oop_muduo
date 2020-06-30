
#ifndef _MUDUO_SOCKET_H_
#define _MUDUO_SOCKET_H_

#include <netinet/in.h>


namespace muduo
{
namespace sockets
{

struct sockaddr_in6 getLocalAddr(int sockfd);
struct sockaddr_in6 getPeerAddr(int sockfd);
int createNonBlockSocket(sa_family_t family);
int connect(int sockfd, const struct sockaddr* addr);
void close(int sockfd);
int getSocketError(int sockfd);
bool isSelfConnect(int sockfd);
void shutdownWrite(int sockfd);

}
}
#endif  // _MUDUO_SOCKET_H_

