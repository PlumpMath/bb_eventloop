
add_definitions(-Wall -Werror -Wextra -D_GNU_SOURCE -Wno-missing-field-initializers -std=c++11)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(libEventLoop_la_SOURCES Abort.cpp
                            AsyncRunner.cpp
                            BaseEpollRunner.cpp
                            CallbackQueue.cpp
                            Callee.cpp
                            ChildProcess.cpp
                            ControlMessage.cpp
                            DumpCreator.cpp
                            DumpFormatter.cpp
                            DumpUtils.cpp
                            Epoll.cpp
                            EpollRunner.cpp
                            FDListener.cpp
                            FDListenerMap.cpp
                            FDMonitor.cpp
                            FileDescriptor.cpp
                            ListeningSocket.cpp
                            MockableRunner.cpp
                            Runner.cpp
                            SeqPacketSocket.cpp
                            SignalSet.cpp
                            SigPipeSuppressor.cpp
                            SocketAddress.cpp
                            StreamBuffer.cpp
                            StreamSocket.cpp
                            System.cpp
                            SystemException.cpp
                            Timer.cpp
                            TimerQueue.cpp
                            TypeName.cpp
                            UncaughtExceptionHandler.cpp)

add_library(EventLoop SHARED ${libEventLoop_la_SOURCES})
add_library(EventLoop_static STATIC ${libEventLoop_la_SOURCES})
set_target_properties(EventLoop_static PROPERTIES OUTPUT_NAME "EventLoop")
set_target_properties(EventLoop PROPERTIES VERSION 0.0.0 SOVERSION 0)
find_library(boostthread boost_thread /usr/lib64)
find_library(boostregex boost_regex /usr/lib64)
find_library(boostdate boost_date_time /usr/lib64)
if(boostthread)
    message(STATUS "path " ${boostthread})
    message(STATUS "path " ${boostregex})
    message(STATUS "path " ${boostdate})
endif(boostthread)
target_link_libraries(EventLoop ${boostthread} ${boostregex} ${boostdate})
install(TARGETS EventLoop EventLoop_static
        LIBRARY DESTINATION lib64
        ARCHIVE DESTINATION lib64)
