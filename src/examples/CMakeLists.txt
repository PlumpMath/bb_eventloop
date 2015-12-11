if(EXAMPLE)

add_definitions(-Wall -Werror -Wextra -std=c++11)

include_directories(${CMAKE_SOURCE_DIR}/include/
                    ${CMAKE_SOURCE_DIR}/include/examples)

find_library(boostsystem boost_system /usr/lib64)
if(boostsystem)
    message(STATUS "path " ${boostsystem})
endif(boostsystem)

set(EchoServer1_SOURCES EchoServer1/EchoServer.cpp
                        EchoServer1/main.cpp)
add_executable(EchoServer1 ${EchoServer1_SOURCES})
target_link_libraries(EchoServer1 EventLoop boost_system)

set(EchoServer2_SOURCES EchoServer2/main.cpp
                        EchoServer1/EchoServer.cpp)
add_executable(EchoServer2 ${EchoServer2_SOURCES})
target_link_libraries(EchoServer2 EventLoop boost_system)

set(EchoClient_SOURCE EchoClient/main.cpp)
add_executable(EchoClient ${EchoClient_SOURCE})
target_link_libraries(EchoClient EventLoop boost_system)

set(SeqPacketEchoServer_SOURCE SeqPacketEchoServer/main.cpp)
add_executable(SeqPacketEchoServer ${SeqPacketEchoServer_SOURCE})
target_link_libraries(SeqPacketEchoServer EventLoop boost_system)

set(SeqPacketEchoClient_SOURCE SeqPacketEchoClient/main.cpp)
add_executable(SeqPacketEchoClient ${SeqPacketEchoClient_SOURCE})
target_link_libraries(SeqPacketEchoClient EventLoop boost_system)

install(TARGETS EchoServer1         DESTINATION bin)
install(TARGETS EchoServer2         DESTINATION bin)
install(TARGETS EchoClient          DESTINATION bin)
install(TARGETS SeqPacketEchoServer DESTINATION bin)
install(TARGETS SeqPacketEchoClient DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/EventLoop/ DESTINATION /usr/include/EventLoop)
endif(EXAMPLE)


