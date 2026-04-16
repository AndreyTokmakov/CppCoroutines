/**============================================================================
Name        : URingCoroutine_2.cpp.cpp
Created on  : 18.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : URingCoroutine_2.cpp
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
    constexpr Handle invalidHandle{-1};
    constexpr uint16_t bufferSize{4096};
    constexpr uint16_t queueDepth{256};

    io_uring ring;

    struct Task
    {
        struct promise_type
        {
            struct FinalAwaiter
            {
                bool await_ready() noexcept {
                    return false;
                }

                void await_suspend(std::coroutine_handle<promise_type> hCoro) noexcept
                {
                    const std::coroutine_handle<> cont = hCoro.promise().continuation;
                    hCoro.destroy();
                    if (cont) { /** symmetric transfer **/
                        cont.resume();
                    }
                }

                void await_resume() noexcept {
                }
            };


            std::coroutine_handle<> continuation{};

            Task get_return_object() {
                return Task { *this };
            }

            std::suspend_always initial_suspend() noexcept
            {
                return {};
            }

            FinalAwaiter final_suspend() noexcept {
                return {};
            }

            void return_void() noexcept {
            }

            void unhandled_exception() {
                std::terminate();
            }
        };

        std::coroutine_handle<promise_type> handle{};

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

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        ~Task() = default;

        // Awaitable interface (structured usage)
        bool await_ready() const noexcept {
            return false;
        }

        std::coroutine_handle<> await_suspend(const std::coroutine_handle<> hCoro)
        {
            handle.promise().continuation = hCoro;
            return handle;  // symmetric transfer
        }

        void await_resume() noexcept
        {

        }
    };

    struct Operation
    {
        std::coroutine_handle<> handle {};
        int result { 0 };
    };


    struct AsyncRead
    {
        Operation op;
        Handle fd;
        char* buffer;
        size_t size;

        AsyncRead(const Handle fd, char* buf, size_t size): fd(fd), buffer(buf), size(size)
        {
        }

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            op.handle = hCoro;
            auto* sqe = io_uring_get_sqe(&ring);
            io_uring_prep_read(sqe, fd, buffer, size, 0);
            sqe->user_data = reinterpret_cast<uint64_t>(&op);
        }

        [[nodiscard]]
        ssize_t await_resume() const noexcept {
            return op.result;
        }
    };

    struct AsyncWrite
    {
        Operation op;
        Handle fd;
        const char* buffer;
        size_t size;

        AsyncWrite(const Handle fd, const char* buf, size_t size)
            : fd(fd), buffer(buf), size(size) {}

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            op.handle = hCoro;
            auto* sqe = io_uring_get_sqe(&ring);
            io_uring_prep_write(sqe, fd, buffer, size, 0);
            sqe->user_data = reinterpret_cast<uint64_t>(&op);
        }

        [[nodiscard]]
        ssize_t await_resume() const noexcept {
            return op.result;
        }
    };

    struct AsyncAccept
    {
        Operation op;
        Handle serverFd;
        sockaddr_in addr{};
        socklen_t len { sizeof(addr) };

        explicit AsyncAccept(const Handle fd) : serverFd(fd){
        }

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(const std::coroutine_handle<> hCoro)
        {
            op.handle = hCoro;
            auto* sqe = io_uring_get_sqe(&ring);
            io_uring_prep_accept(sqe, serverFd, reinterpret_cast<sockaddr*>(&addr), &len, 0);
            sqe->user_data = reinterpret_cast<uint64_t>(&op);
        }

        [[nodiscard]]
        int await_resume() const noexcept {
            return op.result;
        }
    };


    Task handleClient(const Handle clientFd)
    {
        std::array<char, bufferSize> buffer{};
        while (true)
        {
            const ssize_t bytesRead = co_await AsyncRead(clientFd, buffer.data(), buffer.size());
            if (bytesRead <= 0)
            {
                ::close(clientFd);
                co_return;
            }
            co_await AsyncWrite(clientFd, buffer.data(), static_cast<size_t>(bytesRead));
        }
    }

    Task acceptLoop(const Handle serverFd)
    {
        while (true)
        {
            const Handle clientFd = co_await AsyncAccept(serverFd);
            if (clientFd >= 0)
            {
                Task clientTask = handleClient(clientFd);
                clientTask.handle.resume();  // detached start
            }
        }
    }

    void eventLoop()
    {
        while (true)
        {
            io_uring_submit(&ring);
            io_uring_cqe* cqe{};
            if (io_uring_wait_cqe(&ring, &cqe) < 0)
                continue;
            auto* op = reinterpret_cast<Operation*>(cqe->user_data);
            op->result = cqe->res;
            io_uring_cqe_seen(&ring, cqe);
            op->handle.resume();  // symmetric transfer handled internally
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
        if (serverFd < 0)
            return;
        const Task serverTask = acceptLoop(serverFd);
        serverTask.handle.resume();
        eventLoop();
    }
}

void StdCoroutines::Networking::URingCoroutine_2::TestAll()
{
    run();
}