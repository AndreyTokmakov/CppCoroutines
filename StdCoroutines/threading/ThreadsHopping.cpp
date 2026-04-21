/**============================================================================
Name        : ThreadsHopping.cpp
Created on  : 21.04.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "Threading.hpp"

#include <iostream>
#include <syncstream>
#include <utility>
#include <thread>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << " [" << std::this_thread::get_id() << "] "

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

            std::suspend_never  initial_suspend() noexcept {
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

    struct ResumeOnNewThread
    {
        bool await_ready() noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> hCoro) noexcept
        {
            std::thread([hCoro]() {
                hCoro.resume();
            }).detach();
        }

        void await_resume() noexcept {
        }
    };


    // The coroutine
    Task demo()
    {
        LOG << "[1] Running on new thread \n";
        co_await ResumeOnNewThread{};
        LOG << "[2] Now on thread \n";
        co_await ResumeOnNewThread{};
        LOG << "[3] Now on thread \n";
    }

}


void StdCoroutines::Threading::ThreadsHopping::TestAll()
{
    LOG << "[main] Thread\n";
    auto task = demo();

    // Give detached threads time to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    /**
    2026-04-21 19:06:40.102606 [139944242325312] [main] Thread
    2026-04-21 19:06:40.102686 [139944242325312] [1] Running on new thread
    2026-04-21 19:06:40.102731 [139944242321152] [2] Now on thread
    2026-04-21 19:06:40.102794 [139944233928448] [3] Now on thread
    **/
}
