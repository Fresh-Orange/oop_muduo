cmake_minimum_required(VERSION 3.5)
project(oop_muduo)

set(CMAKE_CXX_STANDARD 11)

set(CXX_FLAGS
        -g
        # -DVALGRIND
        -DCHECK_PTHREAD_RETURN_VALUE
        -D_FILE_OFFSET_BITS=64
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wno-unused-parameter
        -Wold-style-cast
        -Woverloaded-virtual
        -Wpointer-arith
        -Wshadow
        -Wwrite-strings
        -march=native
        # -MMD
        -std=c++11
        -rdynamic
        )


add_subdirectory(./base)
add_subdirectory(./net)
# add_subdirectory(./net/http)
# add_subdirectory(./net/inspect)
# add_subdirectory(./net/simple_http)

LINK_LIBRARIES(pthread)

#set(CMAKE_INCLUDE_PATH "./base;./net")

link_libraries(mbase mnet)
add_executable(oop_muduo_server EchoServer.cpp)
add_executable(oop_muduo_client EchoClient.cpp)


# SimpleHttpServer

aux_source_directory(net/simple_http http_SRCS)

add_executable(simple_http ${http_SRCS})
target_link_libraries(simple_http mnet)