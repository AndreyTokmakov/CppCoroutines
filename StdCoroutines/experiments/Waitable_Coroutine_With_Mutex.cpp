/**============================================================================
Name        : Waitable_Coroutine_With_Mutex.cpp
Created on  : 01.03.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine Waitable
============================================================================**/

#include "Experiments.h"
#include <chrono>
#include <thread>

namespace
{
    using Utilities::getCurrentTime;
}

namespace
{

    std::mutex mtx;

    struct Task
    {
        struct promise_type
        {
            promise_type() = default;
            // promise_type(std::mutex& m): mtx {m} {}

            std::suspend_always initial_suspend()
            {
                std::println("[{}] [{}] initial_suspend() - acquiring lock (enter)", getCurrentTime(),std::this_thread::get_id());
                mtx.lock();
                std::println("[{}] [{}] initial_suspend() - acquiring lock (exit)", getCurrentTime(),std::this_thread::get_id());
                return {};
            }

            std::suspend_always final_suspend() noexcept
            {
                std::println("[{}] [{}] final_suspend() - releasing lock)", getCurrentTime(),std::this_thread::get_id());
                mtx.unlock();
                return {};
            }

            Task get_return_object()
            {
                std::println("[{}] [{}] initial_suspend() - acquiring lock (enter)", getCurrentTime(),std::this_thread::get_id());
                return Task {};
            }

            void return_void() {
                std::println("[{}] [{}] return_void()", getCurrentTime(),std::this_thread::get_id());
            }

            void unhandled_exception() {
                std::println("[{}] [{}] unhandled_exception()", getCurrentTime(),std::this_thread::get_id());
            }
        };

        Task operator co_await() {
            return *this;
        }
    };

    struct Awaitable
    {
        std::chrono::milliseconds duration;

        explicit Awaitable(const std::chrono::milliseconds d) : duration(d) {
            std::println("[{}] Awaitable::Awaitable({})", getCurrentTime(), duration.count());
        }

        bool await_ready() noexcept
        {
            std::println("[{}] Awaiter::await_ready(), thread_id: {}", getCurrentTime(), std::this_thread::get_id());

            /** TRUE ==> await_resume() or in case FALSE ==> await_suspend() will be called **/
            return false;
        }

        void await_suspend(std::coroutine_handle<Task::promise_type> handle) noexcept
        {
            /** This function will get called if await_ready() return False **/
            /** In the body you can either return an other coroutine_handle type to change the call execution **/
            /** Or you ca return nothing **/

            std::this_thread::sleep_for(duration);
        }

        void await_resume() noexcept
        {
            /** This function will get called whenever the coroutine is resumed
             *  It is the final result of expression 'co_await exe'
             *  It could return a value or nothing **/

            std::println("[{}] Awaiter::await_resume(), thread_id: {}", getCurrentTime(), std::this_thread::get_id());
        }
    };

    Task async_task()
    {
        std::println("[{}] [{}] Starting the async task ....", getCurrentTime(),std::this_thread::get_id());
        co_await Awaitable { std::chrono::seconds(3u) };
        co_return;
    }
}

void StdCoroutines::Experiments::Waitable_Coroutine_With_Mutex::TestAll()
{
    {
        std::jthread t1([] { async_task(); }), t2([] { async_task(); });
    }
    std::println("[{}] [{}] Done", getCurrentTime(),std::this_thread::get_id());
}
