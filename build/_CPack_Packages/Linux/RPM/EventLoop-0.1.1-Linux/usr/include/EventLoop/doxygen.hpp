/**
 * @mainpage
 *
 * @section intro_sec Introduction
 *
 * EventLoop provides C++ framework for building single-threaded asynchronous
 * event-driven applications. Using threads is not prevented, but EventLoop
 * itself does not utilize threads nor does it force applications to use threads
 * (or even to be aware of threads). Multitasking is achieved by using
 * non-blocking system calls.
 *
 * This generic framework includes:
 * - Timers (EventLoop::Timer)
 * - File descriptor listeners (EventLoop::FDListener)
 * - Signal handling (EventLoop::EpollRunner and EventLoop::SignalSet)
 * - Basic tools for building client/server type of communication
 *   (EventLoop::ListeningSocket, EventLoop::StreamSocket and
 *   EventLoop::SeqPacketSocket)
 * - And various classes for easing up the everyday application coding.
 *
 * There are two different runner implementations available:
 * - EventLoop::EpollRunner which is traditional runner using epoll(7) and
 *   taking over control of the application's thread or process (see
 *   @ref EchoServer1 for an example).
 * - EventLoop::AsyncRunner which itself is a watchable file descriptor that can
 *   be used with select(2), poll(2) or epoll(7) and provides easy integration
 *   point for any already existing event-loop implementation (see
 *   @ref EchoServer2 for an example).
 *
 * @section examples_sec Examples
 *
 * There are two example applications demonstrating the usage of
 * EventLoop::Runner, EventLoop::Timer, EventLoop::ListeningSocket and
 * EventLoop::StreamSocket.
 *
 * @subsection EchoServer1
 *
 * EchoServer1 uses EventLoop::EpollRunner with signal handling and implements
 * protocol independent <I>echo server</I>. It accepts new connections on 0-n listening
 * sockets and echoes data received from a connection back to that connection.
 *
 * Idle connections are closed after 10 seconds timeout. Listening sockets are
 * closed after receiving SIGINT (Ctr+C), which also causes the example
 * application to exit.
 *
 * The code is located in EventLoop/src/examples/EchoServer1 directory.
 *
 * @subsection EchoServer2
 *
 * EchoServer2 uses EventLoop::AsyncRunner for implementing the same <I>echo
 * server</I> than in @ref EchoServer1. The difference is that while in
 * @ref EchoServer1 the <I>event loop</I> is implemented using
 * EventLoop::EpollRunner, in @ref EchoServer2 it is implemented with standard
 * poll(2).
 *
 * The code is located in EventLoop/src/examples/EchoServer2 directory.
 *
 * @namespace EventLoop
 *
 * Top-level namespace.
 *
 * @namespace EventLoop::DumpUtils
 *
 * Miscellaneous utilities to be use for creating debug dumps. Prefer using the
 * macros defined in <EventLoop/DumpUtils.hpp> instead of directly using
 * functions in EventLoop::DumpUtils.
 */
