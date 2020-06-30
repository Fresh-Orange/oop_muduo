
#include "HttpServer.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

using namespace muduo;

namespace muduo
{

void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
{
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}


}

HttpServer::HttpServer(EventLoop* pMainLoop,
                       const InetAddress& listenAddr,
                       const std::string& name,
                       int threadnum)
    : TcpServer(pMainLoop, listenAddr, name, threadnum),
      httpCallback_(defaultHttpCallback)
{

}

HttpServer::~HttpServer()
{
}

void HttpServer::start()
{
    TcpServer::start();
}


void HttpServer::delConnection(muduo::TcpLinkSPtr& conn)
{
    LOG_TRACE("HttpServer::delConnection");
    HttpContext* pHttpContext = (HttpContext*)conn->getContext();
    if (pHttpContext != NULL)
    {
        delete pHttpContext;
    }
    TcpServer::delConnection(conn);
}

void HttpServer::rcvMessage(const TcpLinkSPtr& conn,
                            Buffer* buf, Timestamp receiveTime)
{
    HttpContext* context = (HttpContext*)conn->getContext();
    if (context == NULL)
    {
        return;
    }

    if (!context->parseRequest(buf, receiveTime))
    {
       conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
       conn->shutdown();
    }

    if (context->gotAll())
    {
        rcvRequest(conn, context->request());
        context->reset();
    }
}

void HttpServer::rcvRequest(const TcpLinkSPtr& conn, const HttpRequest& req)
{
    const std::string& connection = req.getHeader("Connection");
    bool close = ((connection == "close") ||
                  ((req.getVersion() == HttpRequest::kHttp10)
                  &&(connection != "Keep-Alive")));
                  
    HttpResponse response(close);
    httpCallback_(req, &response);
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    if (response.closeConnection())
    {
        conn->shutdown();
    }
}

void HttpServer::newConnection(int socketfd, const InetAddress &peerAddr) {
    TcpServer::newConnection(socketfd, peerAddr);

    std::string ipPort = listenAddr_.toIpPort();
    char logBuf[64];
    snprintf(logBuf, 64, "ipPort: %s, connect ID: %d", ipPort.c_str(), connId_-1);
    std::string connName = name_ + logBuf;

    assert(name2connection.find(connName) != name2connection.end());
    auto conn = name2connection[connName];
    if (conn->connected())
    {
        HttpContext* pHttpContext = new HttpContext;
        conn->setContext((void *)pHttpContext);
    }
}

void HttpServer::writeComplete(const TcpLinkSPtr &conn) {
    TcpServer::writeComplete(conn);
}

void HttpServer::highWaterMark(const TcpLinkSPtr &conn, size_t highMark) {
    TcpServer::highWaterMark(conn, highMark);
}


