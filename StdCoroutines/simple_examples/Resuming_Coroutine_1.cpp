/**============================================================================
Name        : Resuming_Coroutine_1.cpp
Created on  : 12.13.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine Waitable
============================================================================**/

#include "SimpleCoroutines.h"
#include "Utilities.h"

#include <coroutine>
#include <print>
#include <chrono>
#include <thread>

namespace
{
    auto tid() { return std::this_thread::get_id();}
    auto time() { return Utilities::getCurrentTime();}
}

namespace
{
    struct Event {
        // you could put a description of a specific event in here
    };

    struct TaskPromise
    {
        struct promise_type;
        using coroutine_handle = std::coroutine_handle<promise_type>;

        struct promise_type
        {
            TaskPromise get_return_object()
            {
                std::println("[{}] [{}] \t\tpromise_type::get_return_object()", tid(), time());
                return TaskPromise {coroutine_handle::from_promise(*this)};
            }

            std::suspend_never initial_suspend()
            {
                /// just using one of the standard awaiter types provided by the C++ library: "std::suspend_never"
                /// This causes the coroutine not to suspend itself on startup, which means that before control returns to main()
                /// the coroutine code will actually start running, and print our “hello, world” message.

                /// If we had instead made initial_suspend() return the other standard type, std::suspend_always,
                /// then the newly created coroutine would be suspended at the beginning of its function body, before the
                /// code that prints “hello, world”.

                /// So that message wouldn’t be printed until the next part of main() did something to resume the coroutine.
                /// But we haven’t yet shown how to do that, so for the moment, our example coroutine starts off not suspended,
                /// because that’s the only way it will print our hello message at all.

                std::println("[{}] [{}] \t\tpromise_type::initial_suspend() -> std""suspend_never()", tid(), time());
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                std::println("[{}] [{}] \t\tpromise_type::final_suspend()", tid(), time());
                return {};
            }

            void return_void() {
                std::println("[{}] [{}] \t\tpromise_type::return_void()", tid(), time());
            }

            void unhandled_exception() {
                std::println("[{}] [{}] \t\tpromise_type::unhandled_exception()", tid(), time());
                std::terminate();
            }

            std::suspend_always await_transform(Event) noexcept
            {   // you could write code here that adjusted the main program's data structures
                // to ensure the coroutine would be resumed at the right time
                return {};
            }
        };

        explicit TaskPromise(const coroutine_handle& handle) : handle { handle }
        {
            std::println("[{}] [{}] CoroutinesTask::CoroutinesTask()", tid(), time());
        }

        ~TaskPromise()
        {
            if (handle)
                handle.destroy();
        }

        void resume() const {
            handle.resume();
        }

        coroutine_handle handle;
    };

    TaskPromise makeCoroutine()
    {
        std::println("[{}] [{}] we're about to suspend this coroutine", tid(), time());
        co_await Event{};
        std::println("[{}] [{}] we've successfully resumed the coroutine", tid(), time());
    }
}

void StdCoroutines::Simple::Resuming_Coroutine_1::TestAll()
{
    TaskPromise coro = makeCoroutine();
    std::println("[{}] [{}] we're back in main()", tid(), time());
    coro.resume();
}
