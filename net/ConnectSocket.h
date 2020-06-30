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
/// ConnectSocket类用于客户端管理连接socket，这个类的功能比较特殊
/// 1、ConnectSocket类负责生成socket描述符，并不断尝试连接服务器
/// 2、如果连接成功，ConnectSocket类将描述符传递给TcpLink管理(TcpLink负责close)
/// 3、如果连接失败，ConnectSocket类释放旧描述符并申请新描述符后继续尝试连接服务器
/// 4、ConnectSocket类用于实现一个无阻塞的connect函数
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

    /// socket的连接状态，初始值是Disconnected
    /// 调用connect()函数，如果返回EINPROGRESS，连接状态变成Connecting
    /// 连接描述符可读可写，且没有错误，连接状态变成Connected
    enum States {Stopped, Connecting, Connected};
    const int MaxRetryTimeMs = 30*1000;
    const int InitRetryTimeMs = 500;
  
    LinkOwner* owner_;          // 指向所属的TcpClient对象

    InetAddress serverAddr_;         // 需要连接的目标服务器地址

    States connectState_;            // 记录当前的连接状态

    int retryTimeMs_;                // 每次重试的时间间隔，单位毫秒

};

typedef std::shared_ptr<ConnectSocket> ConnectSocketSPtr;
typedef std::unique_ptr<ConnectSocket> ConnectSocketUPtr;

}
#endif  // _MUDUO_ACCEPTOR_H_
