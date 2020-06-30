set(http_SRCS
        HttpContext.cpp
        HttpRequest.h
        HttpServer.cpp
        HttpResponse.cpp
        )


add_executable(simple_http HttpServer_main.cpp ${http_SRCS})
target_link_libraries(simple_http mnet)