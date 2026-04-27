/**============================================================================
Name        : TcpClientEpoll_Experimental.cpp
Created on  :
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "Networking.hpp"
#include "Utilities.h"

#include <iostream>
#include <print>
#include <syncstream>

#include <coroutine>
#include <utility>
#include <unordered_map>
#include <vector>
#include <stdexcept>

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>



namespace
{
    using Handle = int32_t;
    using size_type = uint32_t;
    constexpr Handle InvalidHandle { -1 };

    Handle seNonBlocking(const Handle sock)
    {
        const Handle flags = ::fcntl(sock, F_GETFL, 0);
        if (flags == InvalidHandle) {
            throw std::runtime_error("fcntl(F_GETFL) failed");
        }
        if (const Handle handle = ::fcntl(sock, F_SETFL, flags | O_NONBLOCK); handle == InvalidHandle) {
            throw std::runtime_error ( "::fcntl() failed" );
        } else {
            return handle;
        }
    }

    struct SocketGuard final
    {
        Handle sock { InvalidHandle };

        explicit SocketGuard(const Handle s): sock {s} { }

        ~SocketGuard()
        {
            if (sock != InvalidHandle) {
                ::close(sock);
            }
        }

        SocketGuard(const SocketGuard&) = delete;
        SocketGuard(SocketGuard&&) noexcept = delete;

        SocketGuard& operator=(const SocketGuard&) = delete;
        SocketGuard& operator=(SocketGuard&&) noexcept = delete;
    };
}

namespace
{
    struct  EventLoop
    {
        constexpr static uint16_t maxEvents { 128 };

        EventLoop()
        {
            fdEpoll = ::epoll_create1(0);
            if (InvalidHandle == fdEpoll) {
                throw std::runtime_error("epoll_create1 failed");
            }
        }

        ~EventLoop()
        {
            if (InvalidHandle != fdEpoll) {
                ::close(fdEpoll);
            }
        }

        void add(const Handle fd, const uint32_t events, void* ptr) const
        {
            epoll_event event { .events = events, .data = epoll_data_t { .ptr = ptr }};
            if (::epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fd, &event) < 0) {
                throw std::runtime_error("epoll_ctl ADD failed");
            }
        }

        void mod(const Handle fd, const uint32_t events, void* ptr)
        {
            epoll_event event { .events = events, .data = epoll_data_t { .ptr = ptr }};
            if (::epoll_ctl(fdEpoll, EPOLL_CTL_MOD, fd, &event) < 0) {
                throw std::runtime_error("epoll_ctl MOD failed");
            }
        }

        void del(const Handle fd) const
        {
            ::epoll_ctl(fdEpoll, EPOLL_CTL_DEL, fd, nullptr);
        }

        void run() const
        {
            std::array<epoll_event, maxEvents> events{};
            while (true)
            {
                const int eventsCount = ::epoll_wait(fdEpoll, events.data(), events.size(), InvalidHandle);
                if (InvalidHandle == eventsCount) {
                    if (errno == EINTR) {
                        continue;
                    }
                    throw std::runtime_error("epoll_wait failed");
                }
                for (int i = 0; i < eventsCount; ++i)
                {
                    const BaseAwaiter* base = static_cast<BaseAwaiter*>(events[i].data.ptr);
                    base->resume(events[i].events);
                }
            }
        }

        struct BaseAwaiter
        {
            EventLoop& loop;
            Handle fd { InvalidHandle };
            std::coroutine_handle<> coroHandle;

            BaseAwaiter(EventLoop &loop, const Handle fd) : loop(loop), fd(fd) {
            }

            void resume(uint32_t) const {
                coroHandle.resume();
            }

            void addEvent(const std::coroutine_handle<> hCoro, const uint32_t events)
            {
                coroHandle = hCoro;
                loop.add(fd, events, this);
            }
        };

        Handle fdEpoll { InvalidHandle };
    };

    struct ConnectAwaiter : EventLoop::BaseAwaiter
    {
        sockaddr_in addr;

        ConnectAwaiter(EventLoop& loop, const  Handle fd, const sockaddr_in& a):
            BaseAwaiter { loop, fd }, addr(a)
        {
            /** **/
        }

        bool await_ready()
        {
            if (const int res = ::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)); res == 0) {
                return true;
            }
            if (errno == EINPROGRESS) {
                return false;
            }
            throw std::runtime_error("connect failed");
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            addEvent(hCoro, EPOLLOUT);
        }

        void await_resume()
        {
            int err;
            socklen_t len = sizeof(err);
            ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

            if (err != 0) {
                throw std::runtime_error("connect error");
            }

            loop.del(fd);
        }
    };

    struct RecvAwaiter : EventLoop::BaseAwaiter
    {
        char* ptrBuffer { nullptr };
        size_t len { 0U };
        ssize_t result { 0 };

        RecvAwaiter(EventLoop& loop, const  Handle fd, char* b, const size_t size)
            : BaseAwaiter { loop, fd }, ptrBuffer { b }, len { size }
        {
            /** **/
        }

        bool await_ready()
        {
            result = ::recv(fd, ptrBuffer, len, 0);
            if (result >= 0) {
                return true;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            }
            throw std::runtime_error("recv failed");
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            addEvent(hCoro, EPOLLIN);
        }

        ssize_t await_resume()
        {
            if (result < 0) {
                result = ::recv(fd, ptrBuffer, len, 0);
            }
            loop.del(fd);
            return result;
        }
    };

    struct SendAwaiter : EventLoop::BaseAwaiter
    {
        const char* ptrBuffer { nullptr };
        size_t len { 0U };
        ssize_t result { 0 };

        SendAwaiter(EventLoop& loop, const Handle fd, const char* data, const size_t size)
            : BaseAwaiter { loop, fd }, ptrBuffer { data }, len { size }
        {
            /** **/
        }

        bool await_ready()
        {
            result = ::send(fd, ptrBuffer, len, 0);
            if (result >= 0) {
                return true;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            }
            throw std::runtime_error("send failed");
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            addEvent(hCoro, EPOLLOUT);
        }

        ssize_t await_resume()
        {
            if (result < 0) {
                result = ::send(fd, ptrBuffer, len, 0);
            }

            loop.del(fd);
            return result;
        }
    };

    struct Task
    {
        struct Promise;
        using promise_type = Promise;

        struct Promise
        {
            Task get_return_object() {
                return Task {
                    std::coroutine_handle<Promise>::from_promise(*this)
                };
            }

            std::suspend_never initial_suspend() noexcept {
                return {};
            }

            std::suspend_never final_suspend() noexcept {
                return {};
            }

            void return_void() {
            }

            void unhandled_exception() {
                std::terminate();
            }
        };

        std::coroutine_handle<Promise> handle { nullptr };

        explicit Task(const std::coroutine_handle<Promise> hCoro) : handle(hCoro) {
        }
    };

    Task client(EventLoop& loop)
    {
        const Handle sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::runtime_error("socket failed");
        }

        SocketGuard guard(sock);
        seNonBlocking(sock);

        sockaddr_in addr { AF_INET, htons(52525)};
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        co_await ConnectAwaiter(loop, sock, addr);

        constexpr std::string_view msg { "hello" };
        co_await SendAwaiter { loop, sock, msg.data(), msg.size() };

        std::array<char, 1024> buffer {};
        const ssize_t bytesReceived = co_await RecvAwaiter { loop, sock, buffer.data(), buffer.size() };

        std::cout << "Received: " << std::string_view(buffer.data(), bytesReceived) << std::endl;
    }
}

void StdCoroutines::Networking::TcpClientEpoll_Experimental::TestAll()
{
    EventLoop loop;
    client(loop);
    client(loop);
    loop.run();
}
