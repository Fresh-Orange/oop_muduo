#ifndef _MUDUO_LINK_OWNER_H_
#define _MUDUO_LINK_OWNER_H_

#include "../base/copyable.h"
#include "LinkedSocket.h"

/// 这个纯虚类被TcpServer和TcpClient继承
/// 这个类定义了两个虚接口newConnection和delConnection
/// 每个TcpLink都包含一个LinkOwner指针用于记录TcpLink所属的服务器或客户端
/// TcpLink通过虚接口newConnection和delConnection通知服务器/客户端连接建立/删除
namespace muduo
{

class InetAddress;
class EventLoop;

class LinkOwner : noncopyable
{
public:

    LinkOwner(){};
    
    virtual ~LinkOwner(){};

    // 新建连接
    virtual void newConnection(int sockfd, const InetAddress& peerAddr) = 0;

    // 关闭连接
    virtual void delConnection(TcpLinkSPtr& conn) = 0;

    // 接收到新消息
    virtual void rcvMessage(const TcpLinkSPtr& conn, Buffer* buf, Timestamp time) = 0;

    // 发送缓冲区已清空
    virtual void writeComplete(const TcpLinkSPtr& conn) = 0;

    // 发送缓冲区已到达高水线
    virtual void highWaterMark(const TcpLinkSPtr& conn, size_t highMark) = 0;
    
    virtual EventLoop* getLoop() const = 0;
};

}
#endif  // _MUDUO_LINK_OWNER_H_
