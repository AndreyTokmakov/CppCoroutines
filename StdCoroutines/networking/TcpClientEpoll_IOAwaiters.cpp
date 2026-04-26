/**============================================================================
Name        : TcpClientEpoll_IOAwaiters.cpp
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

    struct EpollLoop
    {
        EpollLoop()
        {
            fdEpoll = ::epoll_create1(0);
            if (InvalidHandle == fdEpoll) {
                throw std::runtime_error("epoll_create1 failed");
            }
        }

        ~EpollLoop()
        {
            if (InvalidHandle != fdEpoll) {
                ::close(fdEpoll);
            }
        }

        void addOrModify(const Handle fd, const uint32_t events, const std::coroutine_handle<> hCoro)
        {
            epoll_event ev {.events = events, .data = epoll_data_t { .fd =  fd}};
            handlers[fd] = hCoro;

            if (InvalidHandle == ::epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fd, &ev))
            {
                if (errno == EEXIST)
                {
                    if (InvalidHandle == ::epoll_ctl(fdEpoll, EPOLL_CTL_MOD, fd, &ev)) {
                        throw std::runtime_error("epoll_ctl MOD failed");
                    }
                } else {
                    throw std::runtime_error("epoll_ctl ADD failed");
                }
            }
        }

        void modify(const Handle fd, const uint32_t events, const std::coroutine_handle<> hCoro)
        {
            epoll_event ev {.events = events, .data = epoll_data_t { .fd =  fd}};
            handlers[fd] = hCoro;
            if (InvalidHandle == ::epoll_ctl(fdEpoll, EPOLL_CTL_MOD, fd, &ev)) {
                throw std::runtime_error("epoll_ctl MOD failed");
            }
        }

        void remove(const Handle fd)
        {
            ::epoll_ctl(fdEpoll, EPOLL_CTL_DEL, fd, nullptr);
            handlers.erase(fd);
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
                    const Handle fd = events[i].data.fd;
                    if (auto it = handlers.find(fd); it != handlers.end()) {
                        const std::coroutine_handle<> hCoro = it->second;
                        handlers.erase(it);
                        hCoro.resume();
                    }
                }
            }
        }

    private:

        constexpr static uint16_t maxEvents { 64 };

        Handle fdEpoll { InvalidHandle };
        std::unordered_map<int, std::coroutine_handle<>> handlers;
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
                return Task{
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

        std::coroutine_handle<Promise> h_;

        explicit Task(std::coroutine_handle<promise_type> h) : h_(h) {}
    };


    struct AsyncConnect
    {
        EpollLoop& loop;
        int sock;
        sockaddr_in addr;

        bool await_ready()
        {
            int res = ::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            if (res == 0) return true;

            if (errno == EINPROGRESS) {
                return false;
            }

            throw std::runtime_error("connect failed");
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            loop.addOrModify(sock, EPOLLOUT, hCoro);
        }

        void await_resume()
        {
            int err = 0;
            socklen_t len = sizeof(err);

            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
                throw std::runtime_error("connect completion failed");
            }
        }
    };

    struct AsyncRecv
    {
        EpollLoop& loop;
        int sock;
        void* buffer;
        size_t len;
        ssize_t result = 0;

        bool await_ready()
        {
            result = ::recv(sock, buffer, len, 0);
            if (result >= 0) {
                return true;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            }
            throw std::runtime_error("recv failed");
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            loop.addOrModify(sock, EPOLLIN, hCoro);
        }

        ssize_t await_resume()
        {
            result = ::recv(sock, buffer, len, 0);
            if (result < 0) {
                throw std::runtime_error("recv after epoll failed");
            }
            return result;
        }
    };

    struct AsyncSend
    {
        EpollLoop& loop;
        int sock;
        const char* buffer;
        size_t len;

        size_t offset = 0;

        bool await_ready()
        {
            while (offset < len) {
                ssize_t n = ::send(sock, buffer + offset, len - offset, 0);
                if (n > 0) {
                    offset += n;
                    continue;
                }
                if (n == InvalidHandle && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    return false;
                }
                throw std::runtime_error("send failed");
            }

            return true;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            loop.addOrModify(sock, EPOLLOUT, hCoro);
        }

        ssize_t await_resume()
        {
            while (offset < len) {
                ssize_t n = ::send(sock, buffer + offset, len - offset, 0);
                if (n > 0) {
                    offset += n;
                    continue;
                }
                if (n == InvalidHandle && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    // re-arm epoll and suspend again (depends on your loop design)
                    throw std::runtime_error("still not ready (design issue)");
                }
                throw std::runtime_error("send failed");
            }

            return offset;
        }
    };

    Task<> Client(EpollLoop& loop)
    {
        const Handle sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::runtime_error("socket failed");
        }

        seNonBlocking(sock);

        sockaddr_in addr { AF_INET, htons(52525)};
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        co_await AsyncConnect{loop, sock, addr};

        constexpr std::string_view msg { "hello1" };
        co_await AsyncSend { loop, sock, msg.data(), msg.size() };

        std::array<char, 1024> buf {};
        const ssize_t n = co_await AsyncRecv{loop, sock, buf.data(), buf.size()};

        write(1, buf.data(), n);
    }
}

void StdCoroutines::Networking::TcpClientEpoll_IOAwaiters::TestAll()
{
    EpollLoop loop;
    Client(loop);
    loop.run();
}