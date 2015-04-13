cmake_minimum_required(VERSION 3.1)
project(grabFirst)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c)
add_executable(grabFirst ${SOURCE_FILES} grabFirst-server.h grabFirst-server.c grabFirst-client.h grabFirst-client.c grabFirstClient.h grabFirstClient.c grabFirstServer.h grabFirstServer.c)