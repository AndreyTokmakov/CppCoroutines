/**============================================================================
Name        : EpollCoroutine.cpp.cpp
Created on  : 15.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Networking.cpp
============================================================================**/

#include "Networking.hpp"

#include "Utilities.h"
#include <coroutine>

#include <iostream>
#include <syncstream>
#include <print>

#include <queue>
#include <unordered_map>

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '

namespace
{
    using Handle = int;
    constexpr Handle invalidHandle { -1 };
    constexpr uint16_t bufferSize { 1024 };


    struct EventLoop
    {
        constexpr static uint16_t maxEvents { 256 };

        EventLoop()
        {
            epollFd = ::epoll_create1(0);
            if (0 > epollFd) {
                perror("epoll_create1");
                std::exit(1);
            }
        }

        ~EventLoop() {
            ::close(epollFd);
        }

        void waitRead(const Handle fd, const std::coroutine_handle<> hCoro) {
            addUpdateEvent(fd, EPOLLIN, hCoro);
        }

        void waitWrite(const Handle fd, const std::coroutine_handle<> hCoro) {
            addUpdateEvent(fd, EPOLLOUT, hCoro);
        }

        void remove(const int fd)
        {
            ::epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
            handlers.erase(fd);
            ::close(fd);
        }

        void run()
        {
            epoll_event events[maxEvents] {};
            while (true)
            {
                const int numEvents = ::epoll_wait(epollFd, events, maxEvents, -1);
                for (int i = 0; i < numEvents; ++i) { /** std::for_each_n **/
                    const Handle fd = events[i].data.fd;
                    if (auto it = handlers.find(fd); handlers.end() != it) {
                        std::coroutine_handle<> handler = it->second;
                        handlers.erase(it);
                        handler.resume();
                    }
                }
            }
        }

    private:

        void addUpdateEvent(const Handle fd,
                            const uint32_t numEvents,
                            const std::coroutine_handle<> hCoro)
        {
            epoll_event event { .events = numEvents };
            event.data.fd = fd;

            if (auto [ itCoro, newInserted] = handlers.emplace(fd, hCoro); newInserted) {
                ::epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
            } else {
                ::epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event);
            }
        }

    private:

        Handle epollFd { invalidHandle};
        std::unordered_map<Handle, std::coroutine_handle<>> handlers;
    };


    struct Task
    {
        // TODO: using
        struct promise_type
        {
            Task get_return_object() {
                return Task { *this };
            }

            std::suspend_never initial_suspend() noexcept {
                return {};
            }

            std::suspend_never final_suspend() noexcept {
                return {};
            }

            void return_void() noexcept {
            }

            void unhandled_exception() {
                std::terminate();
            }
        };

        ///using promise_type = Promise;

        using handle_t = std::coroutine_handle<promise_type>;

        explicit Task(promise_type& promise) :
            handle { std::coroutine_handle<promise_type>::from_promise(promise) } {
        }

        Task(Task&& other) noexcept : handle { std::exchange(other.handle, nullptr) } {
        }

        Task& operator=(Task&& other) noexcept {
            handle = std::exchange(other.handle, nullptr);
            return *this;
        }

        ~Task() = default;

        handle_t handle;
    };

    EventLoop loop;

    struct AsyncAccept
    {
        Handle serverFd { invalidHandle };
        Handle clientFd { invalidHandle };

        bool await_ready()
        {
            sockaddr_in addr {};
            socklen_t len = sizeof(addr);

            clientFd = ::accept(serverFd, reinterpret_cast<sockaddr*>(&addr), &len);
            if (clientFd >= 0) {
                return true;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            }
            perror("accept");
            return true;
        }


        void await_suspend(const std::coroutine_handle<> hCoro) {
            loop.waitRead(serverFd, hCoro);
        }

        int await_resume()
        {
            if (clientFd >= 0) {
                return clientFd;
            }
            clientFd = ::accept(serverFd, nullptr, nullptr);
            return clientFd;
        }
    };

    struct AsyncRead
    {
        Handle fd { invalidHandle };
        char* buffer { nullptr };
        size_t size { 0 };
        ssize_t bytesRead { 0 };

        bool await_ready()
        {
            bytesRead = ::read(fd, buffer, size);
            if (bytesRead >= 0)
                return true;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            }
            return true;
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            loop.waitRead(fd, hCoro);
        }

        ssize_t await_resume()
        {
            if (bytesRead >= 0)
                return bytesRead;
            bytesRead = ::read(fd, buffer, size);
            return bytesRead;
        }
    };


    struct AsyncWrite
    {
        Handle fd { invalidHandle };
        const char* buffer { nullptr };
        size_t size { 0 };
        ssize_t bytesWritten { 0 };

        bool await_ready()
        {
            if (const ssize_t bytes = ::write(fd, buffer, size); bytes >= 0) {
                bytesWritten = bytes;
                return bytesWritten == size;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            }
            return true;
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            loop.waitWrite(fd, hCoro);
        }

        ssize_t await_resume()
        {
            while (size > bytesWritten)
            {
                const ssize_t bytes = ::write(fd, buffer + bytesWritten, size - bytesWritten);
                if (bytes > 0) {
                    bytesWritten += bytes;
                }
                else if (errno == EAGAIN) {
                    return false;
                }
                else {
                    return false;
                }
            }
            return true;
        }
    };

    Task handleClient(const Handle clientFd)
    {
        ::fcntl(clientFd, F_SETFD, O_NONBLOCK);
        char buffer [bufferSize] {};
        while (true)
        {
            ssize_t bytesRead = co_await AsyncRead { clientFd, buffer, bufferSize };
            if (0 >= bytesRead) {
                loop.remove(clientFd);
                co_return;
            }
            co_await AsyncWrite { clientFd, buffer, static_cast<size_t>(bytesRead) };
        }
    }

    Task acceptLoop(const Handle serverFd)
    {
        ::fcntl(serverFd, F_SETFD, O_NONBLOCK);
        while (true)
        {
            const Handle clientFd = co_await AsyncAccept { serverFd };
            if (clientFd >= 0) {
                LOG << "New connection: " << clientFd << std::endl;
                handleClient(clientFd);
            }
        }
    }

    Handle createServer(const uint16_t port)
    {
        const Handle serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd < 0) {
            perror("socket");
            return invalidHandle;
        }

        constexpr int opt = 1;
        ::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in server { AF_INET, htons(port), {.s_addr = INADDR_ANY}, {}};
        if (::bind(serverFd, reinterpret_cast<sockaddr *>(&server),sizeof(server)) < 0) {
            perror("bind");
            return invalidHandle;
        }

        if (::listen(serverFd, SOMAXCONN) < 0) {
            perror("listen");
            return invalidHandle;
        }

        return serverFd;
    }

    void run()
    {
        const Handle serverFd = createServer(52525);
        LOG << "Listening on " << serverFd << std::endl;

        acceptLoop(serverFd);
        loop.run();
    }
}

void StdCoroutines::Networking::EpollCoroutine_2::TestAll()
{
    run();
}