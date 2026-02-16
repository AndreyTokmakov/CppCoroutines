/**============================================================================
Name        : EpollCoroutine_1_Ex.cpp
Created on  : 15.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
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
    void set_non_blocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    class EventLoop;
    EventLoop* g_loop = nullptr;

    struct Task
    {
        struct promise_type;
        using handle_t = std::coroutine_handle<promise_type>;

        struct promise_type {
            std::coroutine_handle<> continuation{nullptr};

            Task get_return_object() {
                return Task{handle_t::from_promise(*this)};
            }

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            auto final_suspend() noexcept {
                struct Awaiter {
                    bool await_ready() noexcept { return false; }

                    std::coroutine_handle<>
                    await_suspend(handle_t h) noexcept {
                        auto cont = h.promise().continuation;
                        h.destroy();
                        return cont;   // 🔥 symmetric transfer
                    }

                    void await_resume() noexcept {}
                };
                return Awaiter{};
            }

            void return_void() noexcept {}
            void unhandled_exception() { std::terminate(); }
        };

        handle_t coro;

        explicit Task(handle_t h) : coro(h) {}
        Task(Task&& other) noexcept : coro(other.coro) {
            other.coro = nullptr;
        }

        ~Task() = default;

        bool await_ready() noexcept { return false; }

        std::coroutine_handle<>
        await_suspend(std::coroutine_handle<> awaiting) noexcept {
            coro.promise().continuation = awaiting;
            return coro;  // 🔥 direct transfer
        }

        void await_resume() noexcept {}
    };


    struct EventLoop
    {
        EventLoop()
        {
            epoll_fd_ = epoll_create1(0);
            if (epoll_fd_ < 0) {
                perror("epoll_create1");
                std::exit(1);
            }
        }

        ~EventLoop() {
            close(epoll_fd_);
        }

        void register_event(int fd,
                            uint32_t events,
                            std::coroutine_handle<> h) {
            epoll_event ev{};
            ev.events = events | EPOLLONESHOT;
            ev.data.ptr = h.address();

            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
                if (errno == EEXIST)
                    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
                else {
                    perror("epoll_ctl");
                    std::exit(1);
                }
            }
        }

        void run(std::coroutine_handle<> root)
        {
            root.resume();  // стартуем accept_loop

            constexpr int MAX_EVENTS = 64;
            epoll_event events[MAX_EVENTS];

            while (true) {
                int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
                if (n < 0) {
                    if (errno == EINTR) continue;
                    perror("epoll_wait");
                    std::exit(1);
                }

                for (int i = 0; i < n; ++i) {
                    auto h =
                        std::coroutine_handle<>::from_address(
                            events[i].data.ptr);

                    h.resume();  // 🔥 direct resume
                }
            }
        }

    private:
        int epoll_fd_;
    };


    struct AsyncAccept
    {
        int server_fd;
        int client_fd{-1};

        bool await_ready() {
            client_fd = ::accept(server_fd, nullptr, nullptr);
            if (client_fd >= 0)
                return true;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return false;

            perror("accept");
            return true;
        }

        void await_suspend(std::coroutine_handle<> h) {
            g_loop->register_event(server_fd, EPOLLIN, h);
        }

        int await_resume() {
            if (client_fd < 0)
                client_fd = ::accept(server_fd, nullptr, nullptr);
            return client_fd;
        }
    };

    struct AsyncRead
    {
        int fd;
        char* buffer;
        size_t size;
        ssize_t result{0};

        bool await_ready() {
            result = ::read(fd, buffer, size);
            if (result >= 0)
                return true;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return false;

            return true;
        }

        void await_suspend(std::coroutine_handle<> h) {
            g_loop->register_event(fd, EPOLLIN, h);
        }

        ssize_t await_resume() {
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

        bool await_ready() {
            ssize_t n = ::write(fd, buffer, size);
            return n >= 0;
        }

        void await_suspend(std::coroutine_handle<> h) {
            g_loop->register_event(fd, EPOLLOUT, h);
        }

        void await_resume() {}
    };

    Task handle_client(int client_fd)
    {
        char buffer[4096];

        while (true) {
            ssize_t n = co_await AsyncRead{
                client_fd, buffer, sizeof(buffer)
            };

            if (n <= 0)
                break;

            co_await AsyncWrite{
                client_fd, buffer, (size_t)n
            };
        }

        close(client_fd);
        std::cout << "Client disconnected\n";
    }

    Task accept_loop(int server_fd)
    {
        while (true) {
            int client_fd = co_await AsyncAccept{server_fd};
            if (client_fd < 0)
                continue;

            set_non_blocking(client_fd);

            // 🔥 no scheduler, no queue
            handle_client(client_fd).coro.resume();
        }
    }

    void run()
    {
        EventLoop loop;
        g_loop = &loop;

        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        set_non_blocking(server_fd);

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET,SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(52525);
        addr.sin_addr.s_addr = INADDR_ANY;

        bind(server_fd, (sockaddr*)&addr, sizeof(addr));
        listen(server_fd, SOMAXCONN);

        auto root = accept_loop(server_fd).coro;

        loop.run(root);
    }
}

void StdCoroutines::Networking::EpollCoroutine_1_Ex::TestAll()
{
    run();
}