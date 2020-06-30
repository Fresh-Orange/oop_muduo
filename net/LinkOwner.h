#ifndef _MUDUO_LINK_OWNER_H_
#define _MUDUO_LINK_OWNER_H_

#include "../base/copyable.h"
#include "LinkedSocket.h"

/// ��������౻TcpServer��TcpClient�̳�
/// ����ඨ����������ӿ�newConnection��delConnection
/// ÿ��TcpLink������һ��LinkOwnerָ�����ڼ�¼TcpLink�����ķ�������ͻ���
/// TcpLinkͨ����ӿ�newConnection��delConnection֪ͨ������/�ͻ������ӽ���/ɾ��
namespace muduo
{

class InetAddress;
class EventLoop;

class LinkOwner : noncopyable
{
public:

    LinkOwner(){};
    
    virtual ~LinkOwner(){};

    // �½�����
    virtual void newConnection(int sockfd, const InetAddress& peerAddr) = 0;

    // �ر�����
    virtual void delConnection(TcpLinkSPtr& conn) = 0;

    // ���յ�����Ϣ
    virtual void rcvMessage(const TcpLinkSPtr& conn, Buffer* buf, Timestamp time) = 0;

    // ���ͻ����������
    virtual void writeComplete(const TcpLinkSPtr& conn) = 0;

    // ���ͻ������ѵ����ˮ��
    virtual void highWaterMark(const TcpLinkSPtr& conn, size_t highMark) = 0;
    
    virtual EventLoop* getLoop() const = 0;
};

}
#endif  // _MUDUO_LINK_OWNER_H_
