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


int main() {

    EventLoop* loop = new EventLoop();
    InetAddress local(8080);
    TcpServer* server = new TcpServer(loop, local, "server", 0);

    server->setMessageCallback(onMessage);
    server->setConnectionCallback(onConnect);

    server->start();
    loop->loop();

    return 0;
}
