#ifndef _MUDUO_TCPCLIENT_H_
#define _MUDUO_TCPCLIENT_H_

#include "LinkOwner.h"
#include "EventLoop.h"
#include "ConnectSocket.h"

#include <string>

namespace muduo
{

class TcpClient : public LinkOwner
{
public:

    TcpClient(EventLoop* pMainLoop, const InetAddress& serverAddr,
              const std::string& nameArg);

    ~TcpClient();  

    void connect();

    void disconnect();

    void stop();

    void enableRetry() { retry_ = true; }

    bool retry() const { return retry_; }

    EventLoop* getLoop()  const override { return base_loop_; }

    void newConnection(int sockfd, const InetAddress& peerAddr);

    void delConnection(TcpLinkSPtr& conn);
    
    void rcvMessage(const TcpLinkSPtr& conn, Buffer* buf, Timestamp time);

    void writeComplete(const TcpLinkSPtr& conn);

    void highWaterMark(const TcpLinkSPtr& conn, size_t highMark);

    /// Not thread safe.
    void setConnectionCallback(NewConnectionCallback cb)
    { newConnectionCallback_ = std::move(cb); }

    /// Set message callback.
    /// Not thread safe.
    void setMessageCallback(RcvMessageCallback cb)
    { rcvMessageCallback_ = std::move(cb); }

    /// Set write complete callback.
    /// Not thread safe.
    void setWriteCompleteCallback(WriteCompleteCallback cb)
    { writeCompleteCallback_ = std::move(cb); }

private:

    NewConnectionCallback newConnectionCallback_; // �½����ӻص�����
    
    DelConnectionCallback delConnectionCallback_; // �ر����ӻص�����
    
    RcvMessageCallback    rcvMessageCallback_;    // ���յ�����Ϣ�ص�����

    WriteCompleteCallback writeCompleteCallback_; // ��Ϣ������ɻص�����

    HighWaterMarkCallback highWaterMarkCallback_; //
    
private:

    EventLoop* base_loop_;     // TcpClient�����̵߳�EventLoop
    InetAddress serverAddr_;   // ��Ҫ���ӵ�Ŀ���������ַ
    const std::string name_;   // �ͻ�������
    bool retry_;               // �ͻ������ӱ����Ͽ����Ƿ���Ҫ������������
    bool connect_;             // ��¼�ͻ����Ƿ������Ͽ�
    mutable MutexLock mutex_;
    ConnectSocketUPtr connectSocket_; // �ͻ��������׽���
    int nextLinkId_;          // ÿ��������Ҫ��ͬ��ID��Ϊ����
    TcpLinkSPtr tcpLinkSPtr_;  // ��¼���Ӷ����(����)ָ��
};

}
#endif  // _MUDUO_TCPCLIENT_H_
