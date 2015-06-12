cmake_minimum_required(VERSION 3.1)
project(grabFirstServer)

add_definitions(-lpthread)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread")

set(SOURCE_FILES grabFirstServer.c)
add_executable(grabFirstServer ${SOURCE_FILES} grabProtocolTranslator.h grabProtocolTranslator.c SimpleLogger.c SimpleLogger.h)