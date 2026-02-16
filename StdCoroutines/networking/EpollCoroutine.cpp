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

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '

namespace example_one
{
    void set_non_blocking(const int fd)
    {
        const int flags = ::fcntl(fd, F_GETFL, 0);
        ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    struct Task
    {
        struct promise_type
        {
            using handle_t = std::coroutine_handle<promise_type>;

            struct FinalAwaiter
            {
                bool await_ready() noexcept{
                    return false;
                }

                void await_suspend(handle_t handle) noexcept {
                    handle.destroy();
                }

                void await_resume() noexcept {
                }
            };

            Task get_return_object() {
                return Task { *this };
            }

            std::suspend_always initial_suspend() noexcept {
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


    struct EventLoop
    {
        constexpr static int eventsMax { 64 };

        EventLoop()
        {
            epoll_fd_ = ::epoll_create1(0);
            if (epoll_fd_ < 0) {
                perror("epoll_create1");
                std::exit(1);
            }
        }

        ~EventLoop() {
            ::close(epoll_fd_);
        }

        void schedule(const std::coroutine_handle<> hCoro) {
            readyQueue.push(hCoro);
        }

        void registerEvent(const int fd,
                           const uint32_t events,
                           const std::coroutine_handle<> hCoro)
        {
            epoll_event ev{};
            ev.events = events | EPOLLONESHOT;
            ev.data.ptr = hCoro.address();

            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
                if (errno == EEXIST) {
                    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
                } else {
                    perror("epoll_ctl");
                    std::exit(1);
                }
            }
        }

        void run()
        {
            epoll_event events[eventsMax];
            while (true)
            {
                while (!readyQueue.empty()) {
                    auto h = readyQueue.front();
                    readyQueue.pop();
                    if (!h.done())
                        h.resume();
                }

                const int n = epoll_wait(epoll_fd_, events, eventsMax, -1);
                if (n < 0) {
                    if (errno == EINTR) continue;
                    perror("epoll_wait");
                    std::exit(1);
                }

                for (int i = 0; i < n; ++i) {
                    const std::coroutine_handle<> hCoro = std::coroutine_handle<>::from_address(events[i].data.ptr);
                    schedule(hCoro);
                }
            }
        }

    private:
        int epoll_fd_;
        std::queue<std::coroutine_handle<>> readyQueue;
    };

    EventLoop loop;

    struct AsyncAccept
    {
        int server_fd;
        int clientFd { -1 };

        bool await_ready()
        {
            clientFd = ::accept(server_fd, nullptr, nullptr);
            LOG << "Connection request " << clientFd << std::endl;
            if (clientFd >= 0)
                return true;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                LOG << "errno == EAGAIN || errno == EWOULDBLOCK"<< std::endl;
                return false;
            }
            perror("accept");
            return true;
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            loop.registerEvent(server_fd, EPOLLIN, hCoro);
        }

        int await_resume()
        {
            if (clientFd < 0)
                clientFd = ::accept(server_fd, nullptr, nullptr);
            return clientFd;
        }
    };

    struct AsyncRead
    {
        int fd;
        char* buffer;
        size_t size;
        ssize_t result = 0;

        bool await_ready()
        {
            result = ::read(fd, buffer, size);
            if (result >= 0) {
                LOG << "Request: " << std::string_view(buffer, result);;
                return true;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return false;

            return true;
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            loop.registerEvent(fd, EPOLLIN, hCoro);
        }

        ssize_t await_resume()
        {
            if (result < 0)
                result = ::read(fd, buffer, size);
            return result;
        }
    };

    struct AsyncWrite
    {
        int fd;
        const char* buffer;
        size_t size;
        ssize_t result = 0;

        bool await_ready()
        {
            result = ::write(fd, buffer, size);
            LOG << result << " bytes send back to client\n";

            if (result >= 0)
                return true;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return false;
            return true;
        }

        void await_suspend(const std::coroutine_handle<> hCoro) {
            loop.registerEvent(fd, EPOLLOUT, hCoro);
        }

        ssize_t await_resume()
        {
            if (result < 0)
                result = ::write(fd, buffer, size);
            return result;
        }
    };

    Task handleClient(const int clientFd)
    {
        char buffer[4096];

        while (true)
        {
            const ssize_t bytesRead = co_await AsyncRead { clientFd, buffer, sizeof(buffer) };
            if (bytesRead <= 0)
                break;

            size_t total = 0;
            while (total < static_cast<size_t>(bytesRead))
            {
                const ssize_t bytesWritten = co_await AsyncWrite{clientFd, buffer + total,static_cast<size_t>(bytesRead) - total };
                if (bytesWritten <= 0)
                    break;
                total += bytesWritten;
            }
        }

        ::close(clientFd);
        LOG << "Client disconnected" << std::endl;
    }

    Task acceptLoop(const int serverFd)
    {
        while (true)
        {
            const int clientFd = co_await AsyncAccept { serverFd };
            if (clientFd < 0)
                continue;

            set_non_blocking(clientFd);
            loop.schedule(handleClient(clientFd).handle);
        }
    }


    void run()
    {
        const int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd < 0) {
            perror("socket");
            return ;
        }

        set_non_blocking(serverFd);

        constexpr int opt = 1;
        ::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in server { AF_INET, htons(52525), {.s_addr = INADDR_ANY}, {}};
        if (::bind(serverFd, reinterpret_cast<sockaddr *>(&server),sizeof(server)) < 0) {
            perror("bind");
            return ;
        }

        if (::listen(serverFd, SOMAXCONN) < 0) {
            perror("listen");
            return ;
        }

        loop.schedule(acceptLoop(serverFd).handle);
        loop.run();
    }
}

void StdCoroutines::Networking::EpollCoroutine::TestAll()
{
    example_one::run();
}