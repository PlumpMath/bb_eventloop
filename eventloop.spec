Name:           eventloop 
Version:        0.1.0
Release:        1%{?dist}
Summary:        Eventloop - Generic asynchronous event loop

License:        Nokia
Source0:        %{name}-%{version}.tar.gz
Vendor:		Nokia

BuildRequires:  autoconf, gcc-c++, libtool

%description
Provides generic asynchronous event loop

%prep
%setup -q
./autogen.sh

%build
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
%make_install

%files
/usr/bin/EchoClient
/usr/bin/EchoServer1
/usr/bin/EchoServer2
/usr/bin/SeqPacketEchoClient
/usr/bin/SeqPacketEchoServer
/usr/include/EventLoop/AsyncRunner.hpp
/usr/include/EventLoop/Callee.hpp
/usr/include/EventLoop/ChildProcess.hpp
/usr/include/EventLoop/ControlMessage.hpp
/usr/include/EventLoop/DumpCreator.hpp
/usr/include/EventLoop/DumpUtils.hpp
/usr/include/EventLoop/Dumpable.hpp
/usr/include/EventLoop/EpollRunner.hpp
/usr/include/EventLoop/Exception.hpp
/usr/include/EventLoop/FDListener.hpp
/usr/include/EventLoop/FileDescriptor.hpp
/usr/include/EventLoop/ListeningSocket.hpp
/usr/include/EventLoop/PostponeDeletion.hpp
/usr/include/EventLoop/Runner.hpp
/usr/include/EventLoop/SeqPacketSocket.hpp
/usr/include/EventLoop/SignalSet.hpp
/usr/include/EventLoop/SocketAddress.hpp
/usr/include/EventLoop/StreamBuffer.hpp
/usr/include/EventLoop/StreamSocket.hpp
/usr/include/EventLoop/SystemException.hpp
/usr/include/EventLoop/Timer.hpp
/usr/include/EventLoop/TimerPosition.hpp
/usr/include/EventLoop/TypeName.hpp
/usr/include/EventLoop/UncaughtExceptionHandler.hpp
/usr/include/EventLoop/WeakCallback.hpp
/usr/include/EventLoop/tst/MockableRunner.hpp
/usr/lib64/libEventLoop.a
/usr/lib64/libEventLoop.la
/usr/lib64/libEventLoop.so
/usr/lib64/libEventLoop.so.0
/usr/lib64/libEventLoop.so.0.0.0
/usr/lib64/pkgconfig/libEventLoop.pc

%doc



%changelog
* Thu Sep 24 2015 my spam
- 
