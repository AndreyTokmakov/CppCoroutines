/**============================================================================
Name        : TcpClientEpoll_IOAwaiters_NoAlloc.cpp
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

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

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
    struct FdContext
    {
        Handle fd { InvalidHandle };
        uint32_t events { 0 };
        std::coroutine_handle<> coroHandle {};
    };

    struct Reactor
    {
        Handle fdEpoll { InvalidHandle };
        constexpr static uint16_t maxEvents { 128 };

        Reactor()
        {
            fdEpoll = ::epoll_create1(0);
            if (InvalidHandle == fdEpoll) {
                throw std::runtime_error("epoll_create1 failed");
            }
        }

        ~Reactor()
        {
            if (InvalidHandle != fdEpoll) {
                ::close(fdEpoll);
            }
        }

        void add(FdContext* ctx, const uint32_t ev)
        {
            ctx->events = ev;
            epoll_event e{};
            e.events = ev;
            e.data.ptr = ctx;

            if (::epoll_ctl(fdEpoll, EPOLL_CTL_ADD, ctx->fd, &e) < 0)
            {
                if (errno == EEXIST)
                {
                    if (::epoll_ctl(fdEpoll, EPOLL_CTL_MOD, ctx->fd, &e) < 0) {
                        throw std::runtime_error("epoll mod");
                    }
                } else {
                    throw std::runtime_error("epoll add");
                }
            }
        }

        void mod(FdContext* ctx, const uint32_t ev)
        {
            ctx->events = ev;
            epoll_event event { .events = ev, .data = epoll_data_t { .ptr = ctx }};
            if (InvalidHandle == ::epoll_ctl(fdEpoll, EPOLL_CTL_MOD, ctx->fd, &event)) {
                throw std::runtime_error("epoll_ctl MOD failed");
            }
        }

        void del(const FdContext* ctx) {
            ::epoll_ctl(fdEpoll, EPOLL_CTL_DEL, ctx->fd, nullptr);
        }

        void run()
        {
            std::array<epoll_event, maxEvents> events {};
            while (true)
            {
                const int eventsCount = ::epoll_wait(fdEpoll, events.data(), events.size(), InvalidHandle);
                if (InvalidHandle == eventsCount) {
                    if (errno == EINTR) {
                        continue;
                    }
                    std::cerr << "epoll_wait error\n";
                    break;
                }
                for (int i = 0; i < eventsCount; ++i)
                {
                    FdContext* ctx = static_cast<FdContext*>(events[i].data.ptr);
                    if (events[i].events & (EPOLLERR | EPOLLHUP))
                    {
                        ctx->coroHandle.resume();
                        continue;
                    }
                    const std::coroutine_handle<> hCoro = ctx->coroHandle;
                    ctx->coroHandle = nullptr;
                    hCoro.resume();
                }
            }
        }
    };

    template<typename T = void>
    struct Task;

    template<>
    struct Task<void>
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

            std::suspend_always final_suspend() noexcept {
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

    struct AsyncRecv
    {
        Reactor& reactor;
        FdContext& ctx;
        char* ptrBuffer { nullptr };
        size_t len { 0 };
        ssize_t total { 0 };

        bool await_ready() {
            return false;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            ctx.coroHandle = hCoro;
            reactor.add(&ctx, EPOLLIN);
        }

        ssize_t await_resume()
        {
            while (true)
            {
                if (const ssize_t bytes = recv(ctx.fd, ptrBuffer + total, len - total, 0); bytes > 0) {
                    total += bytes;
                    continue;
                }
                else if (bytes == 0) {
                    return total;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return total;
                }
                throw std::runtime_error("recv");
            }
        }
    };

    struct AsyncSend
    {
        Reactor& reactor;
        FdContext& ctx;
        const char* ptrBuffer { nullptr };
        size_t len { 0 };
        size_t offset { 0 };

        bool await_ready() {
            return false;
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            ctx.coroHandle = hCoro;
            reactor.add(&ctx, EPOLLOUT);
        }

        size_t await_resume()
        {
            for (ssize_t bytes = 0; offset < len; offset += bytes)
            {
                if (bytes = ::send(ctx.fd, ptrBuffer + offset, len - offset, 0); bytes > 0) {
                    continue;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return offset;
                throw std::runtime_error("send");
            }
            return offset;
        }
    };

    // Construct with hostPort ?
    struct AsyncConnect
    {
        Reactor& reactor;
        FdContext& ctx;
        sockaddr_in addr;

        bool await_ready()
        {
            while (true)
            {
                if (const int res = ::connect(ctx.fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)); res == 0) {
                    return true;
                }
                if (errno == EINTR) {
                    continue;
                }
                if (errno == EINPROGRESS) {
                    return false;
                }
                throw std::runtime_error("connect");
            }
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            ctx.coroHandle = hCoro;
            reactor.add(&ctx, EPOLLOUT);
        }

        void await_resume()
        {
            int32_t err = 0;
            socklen_t len = sizeof(err);
            ::getsockopt(ctx.fd, SOL_SOCKET, SO_ERROR, &err, &len);
            if (err != 0) {
                throw std::runtime_error("connect failed");
            }
        }
    };

    Task<> client(Reactor& reactor)
    {
        const Handle sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::runtime_error("socket failed");
        }

        SocketGuard sockGuard { sock };
        seNonBlocking(sock);

        sockaddr_in addr { AF_INET, htons(52525)};
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        FdContext ctx { .fd = sock};
        co_await AsyncConnect { reactor, ctx, addr };

        constexpr std::string_view msg { "hello1" };
        co_await AsyncSend{reactor, ctx, msg.data(), msg.size() };

        std::array<char, 1024> buf {};
        const ssize_t n = co_await AsyncRecv { reactor, ctx, buf.data(), buf.size() };

        write(1, buf.data(), n);
    }
}

void StdCoroutines::Networking::TcpClientEpoll_IOAwaiters_NoAlloc::TestAll()
{
    Reactor reactor;
    Task task = client(reactor);
    reactor.run();
}