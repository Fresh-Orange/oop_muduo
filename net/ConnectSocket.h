#ifndef _MUDUO_CONNECT_H_
#define _MUDUO_CONNECT_H_

#include "FdHandler.h"
#include "../base/InetAddress.h"
#include "../base/Atomic.h"
#include "LinkOwner.h"

#include <map>

namespace muduo
{


class TcpClient;

///
/// ConnectSocket�����ڿͻ��˹�������socket�������Ĺ��ܱȽ�����
/// 1��ConnectSocket�ฺ������socket�������������ϳ������ӷ�����
/// 2��������ӳɹ���ConnectSocket�ཫ���������ݸ�TcpLink����(TcpLink����close)
/// 3���������ʧ�ܣ�ConnectSocket���ͷž���������������������������������ӷ�����
/// 4��ConnectSocket������ʵ��һ����������connect����
class ConnectSocket : public FdHandler
{
public:

    ConnectSocket(LinkOwner* owner, EventLoop* loop, const InetAddress& serverAddr);

    virtual ~ConnectSocket();
    
    void start();

    void stop();  
    
    void restart();

    void handleWrite() override ;

    void handleRead(Timestamp& timestamp) override {
    }

    void handleClose() override {
    }

    void handleError() override {
    }


private:

    void startInLoop();

    void stopInLoop();

    void connect();

    void retry();
    
private:

    /// socket������״̬����ʼֵ��Disconnected
    /// ����connect()�������������EINPROGRESS������״̬���Connecting
    /// �����������ɶ���д����û�д�������״̬���Connected
    enum States {Stopped, Connecting, Connected};
    const int MaxRetryTimeMs = 30*1000;
    const int InitRetryTimeMs = 500;
  
    LinkOwner* owner_;          // ָ��������TcpClient����

    InetAddress serverAddr_;         // ��Ҫ���ӵ�Ŀ���������ַ

    States connectState_;            // ��¼��ǰ������״̬

    int retryTimeMs_;                // ÿ�����Ե�ʱ��������λ����

};

typedef std::shared_ptr<ConnectSocket> ConnectSocketSPtr;
typedef std::unique_ptr<ConnectSocket> ConnectSocketUPtr;

}
#endif  // _MUDUO_ACCEPTOR_H_
