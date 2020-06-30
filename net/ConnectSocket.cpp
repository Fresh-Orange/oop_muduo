
#include "ConnectSocket.h"
#include "../base/InetAddress.h"
#include "../base/Socket.h"
#include "../base/Logging.h"
#include "EventLoop.h"

using namespace muduo;

ConnectSocket::ConnectSocket(LinkOwner* pTcpClient, EventLoop* loop, const InetAddress& serverAddr)
    : FdHandler(loop, -1), // 实际的描述符需要根据连接情况动态创建和释放
      connectState_(Stopped)
{
    owner_ = pTcpClient;
    serverAddr_  = serverAddr;
    retryTimeMs_ = InitRetryTimeMs;
}

ConnectSocket::~ConnectSocket()
{
    
}

// 此函数的调用线程有可能不是ConnectSocket的工作线程
// 如果要操作状态变量必须是原子操作
void ConnectSocket::start()
{
    loop_->runInLoop(std::bind(&ConnectSocket::startInLoop, this));
}

// 此函数的调用线程有可能不是ConnectSocket的工作线程
// 如果要操作状态变量必须是原子操作
void ConnectSocket::stop()
{
    loop_->queueInLoop(std::bind(&ConnectSocket::stopInLoop, this));
}

// 下面的函数一定是在ConnectSocket的工作线程中被调用，
// 所以，下列函数之间不会互相打断，一定是顺序执行的

// 此函数被TcpClient::delConnection调用，两者应该在同一个工作线程
void ConnectSocket::restart()
{
    retryTimeMs_ = InitRetryTimeMs;
    connect();
}

void ConnectSocket::startInLoop()
{
    if (connectState_ == Stopped)
    {
        connect();
    }
}

void ConnectSocket::stopInLoop()
{
    // 只处理这一种情况，其它两种状态不需要stop
    if (connectState_ == Connecting)
    {
        connectState_ = Stopped;
        disableAll();
        updateInPoll(DelEvent);
        sockets::close(fd_);
    }
    // FIXME: cancel timer
}

///
/// 非阻塞connect的实现
/// 步骤1: 创建socket，设置非阻塞属性，尝试连接
///        如果返回0，表示connect成功
///        如果返回值小于0，errno为EINPROGRESS, 表示连接已经启动但是尚未完成
///        如果返回值小于0，errno不是EINPROGRESS，表示连接出错了
/// 步骤2：把sockfd加入poll监听集合判断可读和可写
///        1.如果连接建立好了，对方没有数据到达，那么sockfd可写
///        2.如果在poll之前，连接就建立好了，且对方数据已到达,那么sockfd是可读和可写的
///        3.如果连接发生错误，sockfd也是可读和可写的
///        判断connect是否成功，就得区别(2)和(3),方法是调用getsockopt检查是否出错
/// 步骤3：使用getsockopt函数检查错误
void ConnectSocket::connect()
{
    connectState_ = Connecting;
    fd_ = sockets::createNonBlockSocket(serverAddr_.family());
    int ret = sockets::connect(fd_, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            enableWriting(); // 把sockfd加入poll监听集合等待可写状态
            updateInPoll(AddEvent);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry();
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_ERROR("connect error=%d", savedErrno);
            sockets::close(fd_);
            break;

        default:
            LOG_ERROR("Unexpected error=%d", savedErrno);
            sockets::close(fd_);
            break;
    }
}

// 此函数被FdEvent::handleEvent调用，两者应该在同一个工作线程
void ConnectSocket::handleWrite()
{
    if (connectState_ == Connecting)
    {
        // 无论是连接成功或失败，原有的描述符都不需要再监控了
        disableAll();
        updateInPoll(DelEvent);

        // 使用getsockopt函数检查是否有错误，如果有表示真的连接失败
        int err = sockets::getSocketError(fd_);
        if (err)
        {
            LOG_TRACE("connect fail = %s", strerror(err));
            retry();
        }
        else if (sockets::isSelfConnect(fd_))
        {
            LOG_TRACE("Self connect");
            retry();
        }
        else
        {
            connectState_ = Connected;
            
            // 将描述符传递给TcpLink管理(TcpLink负责close)
            owner_->newConnection(fd_, serverAddr_);
        }
    }
}

void ConnectSocket::retry()
{
    connectState_ = Stopped;
    sockets::close(fd_);

    loop_->runAfter(retryTimeMs_/1000.0,
                          std::bind(&ConnectSocket::start, this));

    retryTimeMs_ = std::min(retryTimeMs_ * 2, MaxRetryTimeMs);
}

