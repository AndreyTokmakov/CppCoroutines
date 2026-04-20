/**============================================================================
Name        : Awaitable_Sleep.cpp
Created on  : 20.04.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "SimpleCoroutines.h"
#include <iostream>
#include <syncstream>
#include <thread>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '


namespace
{
    struct FireAndForget
    {
        struct promise_type
        {
            FireAndForget get_return_object() {
                return {};
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
    };

    struct Sleep
    {
        std::chrono::milliseconds duration;

        // Always suspend
        [[nodiscard]]
        bool await_ready() const noexcept {
            return false;
        }

        // Called when coroutine suspends
        void await_suspend(std::coroutine_handle<> handle) const
        {
            std::thread([handle, d = duration]() {
                std::this_thread::sleep_for(d);
                LOG << " [timer done, resuming coroutine]\n";
                handle.resume(); // Resume after delay
            }).detach();
        }

        void await_resume() const noexcept {
        }
    };

    FireAndForget async_example()
    {
        LOG << "Start\n";
        co_await Sleep {std::chrono::milliseconds(1000) };
        LOG << "After 1 second\n";
        co_await Sleep{std::chrono::milliseconds(500)};
        LOG << "After another 0.5 seconds\n";
    }

    void demo()
    {
        async_example();
        LOG << "Main continues immediately!\n";
        // Prevent program from exiting too early
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void StdCoroutines::Simple::Awaitable_Sleep::TestAll()
{
    demo();
}

/*
2026-04-20 21:40:09.372460 Start
2026-04-20 21:40:09.372571 Main continues immediately!
2026-04-20 21:40:10.372701  [timer done, resuming coroutine]
2026-04-20 21:40:10.372811 After 1 second
2026-04-20 21:40:10.873084  [timer done, resuming coroutine]
2026-04-20 21:40:10.873132 After another 0.5 seconds
*/
