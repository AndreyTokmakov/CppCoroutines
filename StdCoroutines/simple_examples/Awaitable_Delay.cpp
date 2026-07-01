/**============================================================================
Name        : Awaitable_Delay.cpp
Created on  : 01.07.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "SimpleCoroutines.hpp"
#include <iostream>
#include <syncstream>
#include <thread>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '

namespace
{
    using namespace std::chrono_literals;

    struct Delay
    {
        std::chrono::milliseconds duration;

        [[nodiscard]]
        bool await_ready() noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) const {
            std::thread([handle, this]() {
                std::this_thread::sleep_for(duration);
                handle.resume();
            }).detach();
        }

        void await_resume() noexcept {
        }
    };

    struct CoroTask
    {
        struct Promise;
        using promise_type = Promise;
        using coroutine_handle = std::coroutine_handle<Promise>;

        struct Promise
        {
            CoroTask get_return_object() {
                return CoroTask { coroutine_handle::from_promise(*this) };
            }

            [[nodiscard]]
            std::suspend_always initial_suspend() const noexcept {
                return {};
            }

            [[nodiscard]]
            std::suspend_always final_suspend() const noexcept {
                return {};
            }

            void unhandled_exception() {
                std::terminate();
            }

            void return_void() {
            }

            [[nodiscard]]
            Delay await_transform(const std::chrono::milliseconds& delay) noexcept {
                return Delay { delay };
            }
        };

        explicit CoroTask(const coroutine_handle& handle) : coroHandle { handle } {
        }

        ~CoroTask() {
            if (coroHandle) {
                coroHandle.destroy();
            }
        }

        void resume() const  {
            if (coroHandle) {
                coroHandle.resume();
            }
        }

    private:

        coroutine_handle coroHandle;
    };

    CoroTask delayedTask()
    {
        LOG << "Task started" << std::endl;
        co_await 250ms;
        LOG<< "Task resumed after delay" << std::endl;
    }
}

void StdCoroutines::Simple::Awaitable_Delay::TestAll()
{
    CoroTask task = delayedTask();
    LOG << "Main function" << std::endl;
    task.resume();
    std::this_thread::sleep_for(std::chrono::seconds(3)); // Ensure main doesn't exit early
}

/**
2026-07-01 17:30:26.050410 Main function
2026-07-01 17:30:26.050482 Task started
2026-07-01 17:30:26.300588 Task resumed after delay
**/