ACLOCAL_AMFLAGS = -I m4

BASE_CPPFLAGS = -Wall -Werror -Wextra -I$(top_srcdir)/include

lib_LTLIBRARIES = libEventLoop.la
libEventLoop_la_SOURCES = \
    src/lib/Abort.cpp \
    src/lib/AsyncRunner.cpp \
    src/lib/BaseEpollRunner.cpp \
    src/lib/CallbackQueue.cpp \
    src/lib/Callee.cpp \
    src/lib/ChildProcess.cpp \
    src/lib/ControlMessage.cpp \
    src/lib/DumpCreator.cpp \
    src/lib/DumpFormatter.cpp \
    src/lib/DumpUtils.cpp \
    src/lib/Epoll.cpp \
    src/lib/EpollRunner.cpp \
    src/lib/FDListener.cpp \
    src/lib/FDListenerMap.cpp \
    src/lib/FDMonitor.cpp \
    src/lib/FileDescriptor.cpp \
    src/lib/ListeningSocket.cpp \
    src/lib/MockableRunner.cpp \
    src/lib/Runner.cpp \
    src/lib/SeqPacketSocket.cpp \
    src/lib/SignalSet.cpp \
    src/lib/SigPipeSuppressor.cpp \
    src/lib/SocketAddress.cpp \
    src/lib/StreamBuffer.cpp \
    src/lib/StreamSocket.cpp \
    src/lib/System.cpp \
    src/lib/SystemException.cpp \
    src/lib/Timer.cpp \
    src/lib/TimerQueue.cpp \
    src/lib/TypeName.cpp \
    src/lib/UncaughtExceptionHandler.cpp \
    include/private/Abort.hpp \
    include/private/BaseEpollRunner.hpp \
    include/private/CallbackQueue.hpp \
    include/private/DumpFormatter.hpp \
    include/private/Epoll.hpp \
    include/private/FDListenerMap.hpp \
    include/private/FDMonitor.hpp \
    include/private/SigPipeSuppressor.hpp \
    include/private/System.hpp \
    include/private/TimerQueue.hpp
libEventLoop_la_CPPFLAGS = \
    $(BASE_CPPFLAGS) -D_GNU_SOURCE -Wno-missing-field-initializers
libEventLoop_la_LDFLAGS =
libEventLoop_la_LIBADD = -lboost_thread -lboost_regex -lboost_date_time

pkgincludedir = $(includedir)/EventLoop
pkginclude_HEADERS = \
    include/EventLoop/AsyncRunner.hpp \
    include/EventLoop/Callee.hpp \
    include/EventLoop/ChildProcess.hpp \
    include/EventLoop/ControlMessage.hpp \
    include/EventLoop/Dumpable.hpp \
    include/EventLoop/DumpCreator.hpp \
    include/EventLoop/DumpUtils.hpp \
    include/EventLoop/EpollRunner.hpp \
    include/EventLoop/Exception.hpp \
    include/EventLoop/FDListener.hpp \
    include/EventLoop/FileDescriptor.hpp \
    include/EventLoop/ListeningSocket.hpp \
    include/EventLoop/PostponeDeletion.hpp \
    include/EventLoop/Runner.hpp \
    include/EventLoop/SeqPacketSocket.hpp \
    include/EventLoop/SignalSet.hpp \
    include/EventLoop/SocketAddress.hpp \
    include/EventLoop/StreamBuffer.hpp \
    include/EventLoop/StreamSocket.hpp \
    include/EventLoop/SystemException.hpp \
    include/EventLoop/Timer.hpp \
    include/EventLoop/TimerPosition.hpp \
    include/EventLoop/TypeName.hpp \
    include/EventLoop/UncaughtExceptionHandler.hpp \
    include/EventLoop/WeakCallback.hpp
pkginclude_tstdir = $(pkgincludedir)/tst
pkginclude_tst_HEADERS = \
    include/EventLoop/tst/MockableRunner.hpp
pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = libEventLoop.pc

libEventLoop.pc: $(top_srcdir)/src/lib/libEventLoop.pc.in
	sed -e 's![@]prefix[@]!$(prefix)!g' \
        -e 's![@]exec_prefix[@]!$(exec_prefix)!g' \
        -e 's![@]includedir[@]!$(includedir)!g' \
        -e 's![@]libdir[@]!$(libdir)!g' \
        -e 's![@]PACKAGE_VERSION[@]!$(PACKAGE_VERSION)!g' \
        -e 's![@]KRB5_LDFLAGS[@]!$(KRB5_LDFLAGS)!g' \
        -e 's![@]KRB5_LIBS[@]!$(KRB5_LIBS)!g' \
        $< > $@

if EXAMPLES # Disable examples with 'configure --disable-examples'

bin_PROGRAMS = \
    EchoServer1 \
    EchoServer2 \
    EchoClient \
    SeqPacketEchoServer \
    SeqPacketEchoClient

EXAMPLE_CPPFLAGS = $(BASE_CPPFLAGS)
EXAMPLE_LDADD = -lEventLoop -lboost_system

EchoServer1_SOURCES = \
    src/examples/EchoServer1/EchoServer.cpp \
    src/examples/EchoServer1/main.cpp \
    include/examples/EchoServer.hpp
EchoServer1_CPPFLAGS = $(EXAMPLE_CPPFLAGS)
EchoServer1_LDFLAGS =
EchoServer1_LDADD = $(EXAMPLE_LDADD)
EXTRA_EchoServer1_DEPENDENCIES = libEventLoop.la

EchoServer2_SOURCES = \
    src/examples/EchoServer2/main.cpp \
    src/examples/EchoServer1/EchoServer.cpp \
    include/examples/EchoServer.hpp
EchoServer2_CPPFLAGS = $(EXAMPLE_CPPFLAGS)
EchoServer2_LDFLAGS =
EchoServer2_LDADD = $(EXAMPLE_LDADD)
EXTRA_EchoServer2_DEPENDENCIES = libEventLoop.la

EchoClient_SOURCES = \
    src/examples/EchoClient/main.cpp
EchoClient_CPPFLAGS = $(EXAMPLE_CPPFLAGS)
EchoClient_LDFLAGS =
EchoClient_LDADD = $(EXAMPLE_LDADD)
EXTRA_EchoClient_DEPENDENCIES = libEventLoop.la

SeqPacketEchoServer_SOURCES = \
    src/examples/SeqPacketEchoServer/main.cpp
SeqPacketEchoServer_CPPFLAGS = $(EXAMPLE_CPPFLAGS)
SeqPacketEchoServer_LDFLAGS =
SeqPacketEchoServer_LDADD = $(EXAMPLE_LDADD)
EXTRA_SeqPacketEchoServer_DEPENDENCIES = libEventLoop.la

SeqPacketEchoClient_SOURCES = \
    src/examples/SeqPacketEchoClient/main.cpp
SeqPacketEchoClient_CPPFLAGS = $(EXAMPLE_CPPFLAGS)
SeqPacketEchoClient_LDFLAGS =
SeqPacketEchoClient_LDADD = $(EXAMPLE_LDADD)
EXTRA_SeqPacketEchoClient_DEPENDENCIES = libEventLoop.la

endif

GMOCK_SRC = \
    3rdparty/gmock/gtest/include/gtest/gtest-death-test.h \
    3rdparty/gmock/gtest/include/gtest/gtest.h \
    3rdparty/gmock/gtest/include/gtest/gtest-message.h \
    3rdparty/gmock/gtest/include/gtest/gtest-param-test.h \
    3rdparty/gmock/gtest/include/gtest/gtest_pred_impl.h \
    3rdparty/gmock/gtest/include/gtest/gtest-printers.h \
    3rdparty/gmock/gtest/include/gtest/gtest_prod.h \
    3rdparty/gmock/gtest/include/gtest/gtest-spi.h \
    3rdparty/gmock/gtest/include/gtest/gtest-test-part.h \
    3rdparty/gmock/gtest/include/gtest/gtest-typed-test.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-death-test-internal.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-filepath.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-internal.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-linked_ptr.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-param-util-generated.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-param-util.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-port.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-string.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-tuple.h \
    3rdparty/gmock/gtest/include/gtest/internal/gtest-type-util.h \
    3rdparty/gmock/gtest/src/gtest.cc \
    3rdparty/gmock/gtest/src/gtest-death-test.cc \
    3rdparty/gmock/gtest/src/gtest-filepath.cc \
    3rdparty/gmock/gtest/src/gtest-internal-inl.h \
    3rdparty/gmock/gtest/src/gtest-port.cc \
    3rdparty/gmock/gtest/src/gtest-printers.cc \
    3rdparty/gmock/gtest/src/gtest-test-part.cc \
    3rdparty/gmock/gtest/src/gtest-typed-test.cc \
    3rdparty/gmock/include/gmock/gmock-actions.h \
    3rdparty/gmock/include/gmock/gmock-cardinalities.h \
    3rdparty/gmock/include/gmock/gmock-generated-actions.h \
    3rdparty/gmock/include/gmock/gmock-generated-function-mockers.h \
    3rdparty/gmock/include/gmock/gmock-generated-matchers.h \
    3rdparty/gmock/include/gmock/gmock-generated-nice-strict.h \
    3rdparty/gmock/include/gmock/gmock.h \
    3rdparty/gmock/include/gmock/gmock-matchers.h \
    3rdparty/gmock/include/gmock/gmock-more-actions.h \
    3rdparty/gmock/include/gmock/gmock-more-matchers.h \
    3rdparty/gmock/include/gmock/gmock-spec-builders.h \
    3rdparty/gmock/include/gmock/internal/gmock-generated-internal-utils.h \
    3rdparty/gmock/include/gmock/internal/gmock-internal-utils.h \
    3rdparty/gmock/include/gmock/internal/gmock-port.h \
    3rdparty/gmock/src/gmock-cardinalities.cc \
    3rdparty/gmock/src/gmock.cc \
    3rdparty/gmock/src/gmock-internal-utils.cc \
    3rdparty/gmock/src/gmock-matchers.cc \
    3rdparty/gmock/src/gmock-spec-builders.cc
check_PROGRAMS = testrunner
testrunner_SOURCES = \
    $(GMOCK_SRC) \
    tst/AsyncRunnerTest.cpp \
    tst/BaseEpollRunnerTest.cpp \
    tst/CallbackQueueTest.cpp \
    tst/CalleeTest.cpp \
    tst/ChildProcessTest.cpp \
    tst/ControlMessageTest.cpp \
    tst/DumpCreatorTest.cpp \
    tst/DumpFormatterTest.cpp \
    tst/DumpTest.cpp \
    tst/DumpUtilsTest.cpp \
    tst/EpollRunnerTest.cpp \
    tst/EpollRunnerWithoutSignalsTest.cpp \
    tst/EpollRunnerWithSignalsTest.cpp \
    tst/EpollTest.cpp \
    tst/ExceptionTest.cpp \
    tst/FDListenerMapTest.cpp \
    tst/FDListenerTest.cpp \
    tst/FDMonitorTest.cpp \
    tst/FileDescriptorTest.cpp \
    tst/ListeningSocketTest.cpp \
    tst/main.cpp \
    tst/PostponeDeletionTest.cpp \
    tst/RunnerTest.cpp \
    tst/SeqPacketSocketMock.cpp \
    tst/SeqPacketSocketTest.cpp \
    tst/SignalSetTest.cpp \
    tst/SocketAddressTest.cpp \
    tst/StreamBufferTest.cpp \
    tst/StreamSocketTest.cpp \
    tst/SystemExceptionTest.cpp \
    tst/SystemTest.cpp \
    tst/TimerQueueTest.cpp \
    tst/TimerTest.cpp \
    tst/TypeNameTest.cpp \
    tst/UncaughtExceptionHandlerTest.cpp \
    tst/WeakCallbackTest.cpp \
    include/private/tst/AsyncRunnerMock.hpp \
    include/private/tst/BaseEpollRunnerMock.hpp \
    include/private/tst/DummyFDListenerMap.hpp \
    include/private/tst/DumpTestHelper.hpp \
    include/private/tst/DumpTest.hpp \
    include/private/tst/EpollRunnerMock.hpp \
    include/private/tst/FDListenerMock.hpp \
    include/private/tst/ListeningSocketMock.hpp \
    include/private/tst/SeqPacketSocketMock.hpp \
    include/private/tst/StreamSocketMock.hpp \
    include/private/tst/SystemMock.hpp \
    include/private/tst/TimerQueueMock.hpp
testrunner_CPPFLAGS = \
    $(BASE_CPPFLAGS) -Wno-missing-field-initializers \
    -I$(top_srcdir)/3rdparty/gmock/gtest/include \
    -I$(top_srcdir)/3rdparty/gmock/gtest \
    -I$(top_srcdir)/3rdparty/gmock/include \
    -I$(top_srcdir)/3rdparty/gmock
testrunner_LDFLAGS = -pthread
testrunner_LDADD = -lEventLoop -lboost_system
EXTRA_testrunner_DEPENDENCIES = libEventLoop.la

test: testrunner
	$(top_srcdir)/run-tests.sh ./$<

clean-local:
	rm -f libEventLoop.pc testreport.xml testreport-valgrind.log testreport-valgrind.xml
