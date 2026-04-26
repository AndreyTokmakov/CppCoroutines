/**============================================================================
Name        : TcpClientEpoll.cpp.cpp
Created on  :
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "Networking.hpp"

#include "Utilities.h"
#include <coroutine>
#include <utility>
#include <exception>

#include <iostream>
#include <syncstream>
#include <coroutine>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <print>

namespace
{
    using Handle = int32_t;

    Handle seNonBlocking(const Handle sock)
    {
        const Handle flags = ::fcntl(sock, F_GETFL, 0);
        if (flags == -1) {
            throw std::runtime_error("fcntl(F_GETFL) failed");
        }
        if (const Handle handle = ::fcntl(sock, F_SETFL, flags | O_NONBLOCK); handle == -1) {
            throw std::runtime_error ( "::fcntl() failed" );
        } else {
            return handle;
        }
    }

    struct EventData
    {
        Handle fd { -1 };
        std::coroutine_handle<> h;
    };

    struct SocketGuard final
    {
        Handle sock { -1 };

        explicit SocketGuard(const Handle s): sock {s} { }

        ~SocketGuard()
        {
            if (sock != -1) {
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

    struct Task
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

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void return_void() {
            }

            void unhandled_exception(){
                std::terminate();
            }
        };

        std::coroutine_handle<promise_type> handle;

        explicit Task(const std::coroutine_handle<Promise> hCoro) : handle(hCoro) {
        }

        ~Task()
        {
            if (handle) {
                handle.destroy();
            }
        }

        Task(Task&& other) noexcept : handle { std::exchange(other.handle, nullptr)} {
        }
        Task(const Task&) = delete;
    };

    Handle epoll_fd;

    struct EpollAwaiter
    {
        Handle fd;
        uint32_t events;

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            EventData* const data = new EventData{fd, hCoro};
            epoll_event ev {
                .events = events | EPOLLONESHOT ,
                .data = epoll_data { .ptr = data }
            };
            if (::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
                if (errno == EEXIST) {
                    if (::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                        delete data;
                    }
                } else {
                    delete data;
                }
            }
        }

        void await_resume() const noexcept {
        }
    };

    Task tcpClient()
    {
        const Handle sock = ::socket(AF_INET, SOCK_STREAM, 0);
        seNonBlocking(sock);
        SocketGuard guard { sock };

        sockaddr_in server { AF_INET, htons(52525)};
        ::inet_pton(AF_INET, "0.0.0.0", &server.sin_addr);

        const Handle res = ::connect(sock, reinterpret_cast<sockaddr*>(&server),sizeof(server));
        if (res < 0 && errno == EINPROGRESS) {
            co_await EpollAwaiter{sock, EPOLLOUT};
        }

        int err = 0;
        socklen_t len = sizeof(err);
        ::getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len);

        if (err != 0) {
            std::cerr << "connect failed: " << strerror(err) << "\n";
            co_return;
        }

        constexpr std::string_view message { "Hello" };

        for (size_t total = 0, size = message.size(); total < size; ) {
            if (const ssize_t bytes = ::send(sock, message.data() + total, size - total, 0); bytes > 0) {
                total += bytes;
            } else if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                co_await EpollAwaiter {sock, EPOLLOUT };
            } else {
                std::cerr << "send error\n";
                co_return;
            }
        }

        std::array<char, 1024> buffer {};
        co_await EpollAwaiter {sock, EPOLLIN };

        ssize_t bytes = ::recv(sock, buffer.data(), buffer.size(), 0);
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                co_await EpollAwaiter{sock, EPOLLIN};
                bytes = ::recv(sock, buffer.data(), buffer.size(), 0);
            } else {
                std::cerr << "recv error\n";
                co_return;
            }
        }
        else if (bytes == 0) {
            std::cout << "connection closed\n";
            co_return;
        }
        else {
            std::cout << "Received: " << std::string_view{ buffer.data(), static_cast<uint32_t>(bytes) } << "\n";

        }
    }

    void eventLoop()
    {
        std::array<epoll_event, 16> events {};
        while (true)
        {
            const int eventsCount = ::epoll_wait(epoll_fd, events.data(), events.size(), -1);
            if (-1 == eventsCount) {
                if (errno == EINTR) {
                    continue;
                }
                std::cerr << "epoll_wait error\n";
                break;
            }
            for (int i = 0; i < eventsCount; ++i) {
                const EventData* data = static_cast<EventData*>(events[i].data.ptr);
                ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, nullptr);
                std::coroutine_handle<> hCoro = data->h;
                delete data;
                hCoro.resume();
            }
        }
    }
}

void StdCoroutines::Networking::TcpClientEpoll::TestAll()
{
    epoll_fd = epoll_create1(0);
    if (-1 == epoll_fd) {
        std::cerr << "epoll_create1 error: " << strerror(errno) << "\n";
        return;
    }
    auto task = tcpClient();
    eventLoop();
}