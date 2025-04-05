/**============================================================================
Name        : Awaiter_Lifecycle_Steps.cpp
Created on  : 13.13.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "SimpleCoroutines.h"

#include <chrono>
#include <thread>
#include <random>
#include <vector>
#include <source_location>

namespace
{
    auto tid() { return std::this_thread::get_id(); }
    auto time() { return Utilities::getCurrentTime(); }


    void log(const std::string_view message = std::string_view(),
             const std::source_location location = std::source_location::current())
    {
        std::println("[{}] [{}] {} {}", tid(), time(), location.function_name(), message);
    }

    void info(const std::string_view message = std::string_view())
    {
        // std::println("[{}] [{}] {}", tid(), time(), message);
        std::println("{}",message);
    }
}

namespace
{
    // https://www.alibabacloud.com/blog/a-comprehensive-discussion-on-c%2B%2B20-coroutines_600889

    template <bool ReadyFlag>
    struct Awaiter
    {
        bool await_ready() noexcept
        {
            info(std::format("  <--- Awaiter::await_ready({})", ReadyFlag));

            /** TRUE ==> await_resume() or in case FALSE ==> await_suspend() will be called **/
            return ReadyFlag;
        }

        void await_resume() noexcept
        {
            info("Awaiter::await_resume()");
        }

        void await_suspend(std::coroutine_handle<>) noexcept
        {
            info("Awaiter::await_suspend()");
        }
    };

    struct TaskPromise
    {
        struct promise_type
         {
            TaskPromise get_return_object()
            {
                info("get_return_object()");
                return TaskPromise{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            Awaiter<true> initial_suspend() noexcept {
                info("initial_suspend()");
                return {};
            }

            Awaiter<true> final_suspend() noexcept {
                info("final_suspend()");
                return {};
            }

            void unhandled_exception() {
                info("unhandled_exception()");
            }

            void return_void() noexcept {
                info("return_void()");
            }
        };

        void resume()
        {
            info("resume()");
            handle.resume();
        }

        std::coroutine_handle<promise_type> handle;
    };

    TaskPromise createCoroutine()
    {
        info("---> Task first run");
        co_await Awaiter<false>{};
        info("---> Task resume");
    }

    /* code after the compiler is expanded can be roughly imagined:
    TaskPromise task_func()
    {
        // No parameters and local variables.
        auto state = new __TaskPromise_state_(); // has TaskPromise::promise_type promise;

        TaskPromise coro = state.promise.get_return_object();

        try
        {
            co_await promise.initial_suspend();

            std::cout << "task first run" << std::endl;

            co_await Awaiter<false>{};

            std::cout << "task resume" << std::endl;
        } catch (...) {
            state.promise.unhandled_exception();
        }

        co_await state.promise.final_suspend();
    }
    */
}

void StdCoroutines::Simple::Awaiter_Lifecycle_Steps::TestAll()
{
    TaskPromise promise = createCoroutine();
    promise.resume();

    info("---> back to main()");

    /**
    get_return_object()
    initial_suspend()
      <--- Awaiter::await_ready(true)
    Awaiter::await_resume()

    ---> Task first run
      <--- Awaiter::await_ready(false)
    Awaiter::await_suspend()
    resume()
    Awaiter::await_resume()

    ---> Task resume
    return_void()
    final_suspend()
      <--- Awaiter::await_ready(true)
    Awaiter::await_resume()

    ---> back to main()

    */
}
