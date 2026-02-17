/**============================================================================
Name        : URingCoroutine.cpp.cpp
Created on  : 16.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : URingCoroutine.cpp
============================================================================**/

#include "Networking.hpp"

#include "Utilities.h"
#include <coroutine>

#include <iostream>
#include <syncstream>
#include <print>

#include <queue>

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
    constexpr uint16_t bufferSize { 4096 };
    constexpr uint16_t maxEvents { 256 };


    struct Reactor
    {
        Reactor()
        {
            epollFd = ::epoll_create1(0);
            if (0 > epollFd) {
                perror("epoll_create1");
                std::exit(1);
            }
        }

        ~Reactor() {
            ::close(epollFd);
        }

        void wait(const Handle fd, const uint32_t eventsNum, void* op_ptr)
        {
            epoll_event event {  .events = eventsNum };
            event.data.ptr = op_ptr ;

            ::epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
        }

        void modify(const Handle fd, const uint32_t eventsNum, void* op_ptr)
        {
            epoll_event event {  .events = eventsNum };
            event.data.ptr = op_ptr ;

            ::epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event);
        }

        void remove(const Handle fd)
        {
            ::epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
            ::close(fd);
        }

        void run()
        {
            std::array<epoll_event, maxEvents> events{};
            while (true)
            {
                const int numEvents  = ::epoll_wait(epollFd, events.data(), maxEvents, -1);
                for (int i = 0; i < numEvents; ++i) {
                    Operation* op = static_cast<Operation*>(events[i].data.ptr);
                    op->resume();
                }
            }
        }

    private:

        struct Operation
        {
            std::coroutine_handle<> handle;

            void resume() const
            {
                handle.resume();
            }
        };

        Handle epollFd { invalidHandle };
    };

    Reactor reactor;


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
            handle { std::coroutine_handle<promise_type>::from_promise(promise) }
        {
            // LOG << "Task created" << std::endl;
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

    struct BaseOp
    {
        Handle fd { invalidHandle };
        std::coroutine_handle<> handle;

        void resume() const
        {
            handle.resume();
        }
    };

    struct AsyncRead: BaseOp
    {
        char* buffer { nullptr };
        size_t size { 0 };
        ssize_t bytesRead { invalidHandle };

        AsyncRead(const Handle fd, char* buff, const size_t size):
            BaseOp { fd }, buffer { buff },  size { size }
        {
        }

        bool await_ready()
        {
            bytesRead = ::read(fd, buffer, size);
            if (bytesRead >= 0) {
                return true;
            }
            return errno != EAGAIN;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            handle = hCoro;
            reactor.wait(fd, EPOLLIN, this);
        }

        [[nodiscard]]
        ssize_t await_resume() const
        {
            if (bytesRead >= 0) {
                return bytesRead;
            }
            return ::read(fd, buffer, size);
        }
    };

    struct AsyncWrite: BaseOp
    {
        const char* buffer { nullptr };
        size_t  sizeToWrite { 0 };
        ssize_t bytesWritten { invalidHandle };

        AsyncWrite(const Handle fd, const char* buff, const size_t size):
            BaseOp { fd }, buffer { buff }, sizeToWrite { size }
        {
            // LOG << "AsyncWrite {size: " << size << "}" << std::endl;
        }

        bool await_ready()
        {
            const ssize_t wBytes = ::write(fd, buffer, sizeToWrite);
            if (wBytes > 0) {
                bytesWritten = wBytes;
                return bytesWritten == sizeToWrite;
            }
            return errno != EAGAIN;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            handle = hCoro;
            reactor.wait(fd, EPOLLOUT, this);
        }

        void await_resume()
        {
            while (sizeToWrite > bytesWritten) {
                const ssize_t wBytes = ::write(fd, buffer + bytesWritten, sizeToWrite - bytesWritten);
                if (wBytes > 0) {
                    bytesWritten += wBytes;
                }
                return;
            }
        }
    };

    struct AsyncAccept: BaseOp
    {
        Handle clientFd { invalidHandle };

        explicit AsyncAccept(const Handle serverFd)
        {
            fd = serverFd;
        }

        bool await_ready()
        {
            clientFd = ::accept(fd, nullptr, nullptr);
            return clientFd >= 0;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            handle = hCoro;
            reactor.wait(fd, EPOLLIN, this);
        }

        [[nodiscard]]
        Handle await_resume() const
        {
            if (clientFd >= 0)
                return clientFd;
            return ::accept(fd, nullptr, nullptr);
        }
    };

    Task handleClient(const Handle clientFd)
    {
        ::fcntl(clientFd, F_SETFD, O_NONBLOCK);
        std::array<char, bufferSize> buffer {};
        while (true)
        {
            const ssize_t bytesRead = co_await AsyncRead { clientFd, buffer.data(), bufferSize };
            if (0 >= bytesRead) {
                reactor.remove(clientFd);
                co_return;
            }
            co_await AsyncWrite { clientFd, buffer.data(), static_cast<size_t>(bytesRead) };
        }
    }

    Task acceptLoop(const Handle serverFd)
    {
        ::fcntl(serverFd, F_SETFD, O_NONBLOCK);
        while (true)
        {
            const Handle clientFd = co_await AsyncAccept { serverFd };
            if (clientFd >= 0) {
                handleClient(clientFd);
            }
        }
    }

    Handle createServer(const uint16_t port)
    {
        const Handle serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (0 >= serverFd) {
            ::perror("socket");
            return invalidHandle;
        }

        constexpr int opt = 1;
        ::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in server { AF_INET, htons(port), {.s_addr = INADDR_ANY}, {}};
        if (::bind(serverFd, reinterpret_cast<sockaddr*>(&server),sizeof(server)) < 0) {
            ::perror("bind");
            return invalidHandle;
        }
        if (::listen(serverFd, SOMAXCONN) < 0) {
            ::perror("listen");
            return invalidHandle;
        }
        return serverFd;
    }

    void run()
    {
        const Handle serverFd = createServer(52525);
        LOG << "Running server on " << serverFd << std::endl;
        acceptLoop(serverFd);
        reactor.run();
    }
}

void StdCoroutines::Networking::EpollCoroutine_LessAlloc::TestAll()
{
    run();
}