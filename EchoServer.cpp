#include <iostream>
#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "base/InetAddress.h"
#include "base/Logging.h"
using namespace muduo;

void onConnect(const TcpLinkSPtr& conn){
    muduo::LOG_INFO("EchoServer - %s  is %s",
                    conn->peerAddr().toIpPort().c_str(),
                    ((conn->connected() ? "UP" : "DOWN")));
}

void onMessage(const TcpLinkSPtr& conn, Buffer* buf, Timestamp t){
    std::string rec = buf->retrieveAllAsString();
    std::cout << "from" << conn->peerAddr().toIpPort() <<" rec buf = " << rec << endl;
    conn->send(buf);
}


int main(int argc, char **argv) {

    if (argc > 2) {
        muduo::LOG_INFO("usage: simple_http [thread_num(default 0)]");
        return -1;
    }

    int thread_num = 0;
    if (argc == 2)
        thread_num = atoi(argv[1]);

    EventLoop* loop = new EventLoop();
    InetAddress local(8080);
    TcpServer* server = new TcpServer(loop, local, "server", thread_num);

    server->setMessageCallback(onMessage);
    server->setConnectionCallback(onConnect);

    server->start();
    loop->loop();

    return 0;
}
