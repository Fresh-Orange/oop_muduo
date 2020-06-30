
#include "ConnectSocket.h"
#include "../base/InetAddress.h"
#include "../base/Socket.h"
#include "../base/Logging.h"
#include "EventLoop.h"

using namespace muduo;

ConnectSocket::ConnectSocket(LinkOwner* pTcpClient, EventLoop* loop, const InetAddress& serverAddr)
    : FdHandler(loop, -1), // ʵ�ʵ���������Ҫ�������������̬�������ͷ�
      connectState_(Stopped)
{
    owner_ = pTcpClient;
    serverAddr_  = serverAddr;
    retryTimeMs_ = InitRetryTimeMs;
}

ConnectSocket::~ConnectSocket()
{
    
}

// �˺����ĵ����߳��п��ܲ���ConnectSocket�Ĺ����߳�
// ���Ҫ����״̬����������ԭ�Ӳ���
void ConnectSocket::start()
{
    loop_->runInLoop(std::bind(&ConnectSocket::startInLoop, this));
}

// �˺����ĵ����߳��п��ܲ���ConnectSocket�Ĺ����߳�
// ���Ҫ����״̬����������ԭ�Ӳ���
void ConnectSocket::stop()
{
    loop_->queueInLoop(std::bind(&ConnectSocket::stopInLoop, this));
}

// ����ĺ���һ������ConnectSocket�Ĺ����߳��б����ã�
// ���ԣ����к���֮�䲻�ụ���ϣ�һ����˳��ִ�е�

// �˺�����TcpClient::delConnection���ã�����Ӧ����ͬһ�������߳�
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
    // ֻ������һ���������������״̬����Ҫstop
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
/// ������connect��ʵ��
/// ����1: ����socket�����÷��������ԣ���������
///        �������0����ʾconnect�ɹ�
///        �������ֵС��0��errnoΪEINPROGRESS, ��ʾ�����Ѿ�����������δ���
///        �������ֵС��0��errno����EINPROGRESS����ʾ���ӳ�����
/// ����2����sockfd����poll���������жϿɶ��Ϳ�д
///        1.������ӽ������ˣ��Է�û�����ݵ����ôsockfd��д
///        2.�����poll֮ǰ�����Ӿͽ������ˣ��ҶԷ������ѵ���,��ôsockfd�ǿɶ��Ϳ�д��
///        3.������ӷ�������sockfdҲ�ǿɶ��Ϳ�д��
///        �ж�connect�Ƿ�ɹ����͵�����(2)��(3),�����ǵ���getsockopt����Ƿ����
/// ����3��ʹ��getsockopt����������
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
            enableWriting(); // ��sockfd����poll�������ϵȴ���д״̬
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

// �˺�����FdEvent::handleEvent���ã�����Ӧ����ͬһ�������߳�
void ConnectSocket::handleWrite()
{
    if (connectState_ == Connecting)
    {
        // ���������ӳɹ���ʧ�ܣ�ԭ�е�������������Ҫ�ټ����
        disableAll();
        updateInPoll(DelEvent);

        // ʹ��getsockopt��������Ƿ��д�������б�ʾ�������ʧ��
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
            
            // �����������ݸ�TcpLink����(TcpLink����close)
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

