
#include "net/simple_http/HttpRequest.h"
#include "net/simple_http/HttpResponse.h"
#include "net/simple_http/HttpServer.h"
#include "base/Timestamp.h"
#include "base/Logging.h"
#include "net/EventLoop.h"

#include <unistd.h>
#include <string>
#include <iostream>
#include <signal.h>

extern char favicon[555];
bool benchmark = false;

void onRequest(const muduo::HttpRequest& req, muduo::HttpResponse* resp)
{
  std::cout << "Headers " << req.methodString() << " " << req.path() << std::endl;
  if (!benchmark)
  {
    const std::map<std::string, std::string>& headers = req.headers();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end();
         ++it)
    {
      std::cout << it->first << ": " << it->second << std::endl;
    }
  }

  if (req.path() == "/")
  {
    resp->setStatusCode(muduo::HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/html");
    resp->addHeader("Server", "Muduo");
    std::string now = muduo::Timestamp::now().toFormattedString();
    resp->setBody("<html><head><title>This is title</title></head>"
        "<body><h1>Hello</h1>Now is " + now +
        "</body></html>");
  }
  else if (req.path() == "/hello")
  {
    resp->setStatusCode(muduo::HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    resp->addHeader("Server", "Muduo");
    resp->setBody("hello, world!\n");
  }
  else
  {
    resp->setStatusCode(muduo::HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
  }
}

int main(int argc, char **argv)
{

    if (argc > 2) {
        muduo::LOG_INFO("usage: simple_http [thread_num(default 0)]");
        return -1;
    }

    int thread_num = 0;
    if (argc == 2)
        thread_num = atoi(argv[1]);

    muduo::LOG_INFO("Hellow HttpServer");
    ::signal(SIGPIPE, SIG_IGN);
    muduo::LOG_TRACE("Ignore SIGPIPE in mail");

    muduo::EventLoop eventLoop;
    muduo::HttpServer httpServer(&eventLoop, muduo::InetAddress(80), "httpServer", thread_num);
    httpServer.setHttpCallback(onRequest);
    httpServer.start();
    eventLoop.loop();
    
    return 0;
}

