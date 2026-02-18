/**============================================================================
Name        : URingCoroutine.cpp.cpp
Created on  : 18.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : URingCoroutine.cpp
============================================================================**/

#include "Networking.hpp"

#include "Utilities.h"
#include <coroutine>
#include <utility>

#include <iostream>
#include <syncstream>
#include <print>

#include <array>
#include <unordered_map>

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <liburing.h>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '

namespace
{
    using Handle = int;
    constexpr Handle  invalidHandle { -1 };
    constexpr uint16_t bufferSize { 1024 * 4 };
    constexpr uint16_t queueDepth { 256 };

    struct Operation
    {
        std::coroutine_handle<> handle;
        int result { invalidHandle };
    };

    io_uring ring;

    void eventLoop()
    {
        while (true)
        {
            io_uring_cqe* cqe;
            if (const int ret = io_uring_wait_cqe(&ring, &cqe); 0 > ret)
                continue;
            Operation* op = reinterpret_cast<Operation*>(cqe->user_data);
            op->result = cqe->res;

            io_uring_cqe_seen(&ring, cqe);
            op->handle.resume();
        }
    }

    struct Task
    {
        struct Promise
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

        using promise_type = Promise;
        using handle_t = std::coroutine_handle<promise_type>;

        explicit Task(promise_type& promise) :
            handle { std::coroutine_handle<promise_type>::from_promise(promise) }
        {
            LOG << "Task created" << std::endl;
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

    struct AsyncWrite: Operation
    {
        Handle fd { invalidHandle };
        const char* buffer { nullptr };
        size_t size { 0 };

        AsyncWrite(const Handle fd, const char* buff, const size_t size):
            fd { fd }, buffer { buff } ,size { size }
        {
            LOG << "AsyncWrite {size: " << size << "}" << std::endl;
        }

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            handle = hCoro;
            auto* sqe = io_uring_get_sqe(&ring);
            io_uring_prep_write(sqe, fd, buffer, size, 0);
            sqe->user_data = reinterpret_cast<unsigned long>(this);
            io_uring_submit(&ring);
        }

        ssize_t await_resume() const noexcept {
            return result;
        }
    };

    struct AsyncRead: Operation
    {
        Handle fd { invalidHandle };
        char* buffer { nullptr };
        size_t size { 0 };

        AsyncRead(const Handle fd, char* buff, const size_t size):
            fd { fd }, buffer { buff } ,size { size }
        {
        }

        [[nodiscard]]
        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            handle = hCoro;
            auto* sqe = io_uring_get_sqe(&ring);
            io_uring_prep_read(sqe, fd, buffer, size, 0);
            sqe->user_data = reinterpret_cast<unsigned long>(this);
            io_uring_submit(&ring);
        }

        [[nodiscard]]
        ssize_t await_resume() const noexcept {
            return result;
        }
    };

    struct AsyncAccept:  Operation
    {
        Handle serverFd { invalidHandle };
        sockaddr_in address {};
        socklen_t len { sizeof(address) };

        explicit AsyncAccept(const Handle fd): serverFd { fd } {
        }

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            handle = hCoro;
            auto* sqe = io_uring_get_sqe(&ring);
            io_uring_prep_accept(sqe, serverFd, reinterpret_cast<sockaddr*>(&address), &len, 0);
            sqe->user_data = reinterpret_cast<unsigned long>(this);
            io_uring_submit(&ring);
        }

        int await_resume() const noexcept {
            return result;
        }
    };

    Task handleClient(const Handle clientFd)
    {
        std::array<char, bufferSize> buffer {};
        while (true)
        {
            const ssize_t bytesRead = co_await AsyncRead { clientFd, buffer.data(), bufferSize };
            if (0 >= bytesRead) {
                ::close(clientFd);
                co_return;
            }
            co_await AsyncWrite { clientFd, buffer.data(), static_cast<size_t>(bytesRead) };
        }
    }

    Task acceptLoop(const Handle serverFd)
    {
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
        io_uring_queue_init(queueDepth, &ring, 0);
        const Handle serverFd = createServer(52525);
        LOG << "Running server on " << serverFd << std::endl;
        acceptLoop(serverFd);
        eventLoop();
    }
}

void StdCoroutines::Networking::URingCoroutine::TestAll()
{
    run();
}