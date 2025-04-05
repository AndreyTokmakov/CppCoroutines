/**============================================================================
Name        : Waitable_Coroutine_2.cpp
Created on  : 28.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine Waitable
============================================================================**/

#include "SimpleCoroutines.h"

#include <chrono>
#include <thread>

/**

Coroutine Functions and Suspension Points
A coroutine function is a special type of function in C++ that can be suspended and resumed at specific points
during its execution.
This is achieved using three key keywords: co_await, co_yield, and co_return.

1. co_await:  This keyword is used to suspend the execution of the coroutine until a particular
              condition is met or an asynchronous operation is completed.

2. co_yield:  This keyword allows the coroutine to produce a value and suspend its execution.
              It can be resumed later, continuing from the point after co_yield.

3. co_return: This keyword is used to return a value from the coroutine and finalize its execution.

Custom Awaitable Types

In C++, there are two built-in types like
-   std::suspend_always
-   std::suspend_never.

We create custom awaitable types that define specific suspension and resumption behavior:
An awaitable type is any type that implements the following methods:

1. await_ready()  : Determines if the coroutine should suspend or continue without suspension.

2. await_suspend(): Defines what happens when the coroutine is suspended.
                    This can involve storing the coroutine handle, initiating an asynchronous operation,
                    or interacting with other coroutines.

3. await_resume() : Defines what happens when the coroutine is resumed, often returning a value or performing
                    some final action before execution continues.

In this example, 'AwaiterTimer' is a custom awaitable type that suspends the coroutine for a specified duration.
The coroutine resumes automatically after the specified time, allowing you to integrate time-based delays
or other asynchronous tasks into your coroutine workflow.

**/

namespace
{
    auto tid() { return std::this_thread::get_id();}
    auto time() { return Utilities::getCurrentTime();}
}

namespace
{
    struct TaskPromise
    {
        struct promise_type
        {
            TaskPromise get_return_object()
            {
                std::println("[{}] [{}] \tpromise_type::get_return_object()", tid(), time());
                return TaskPromise {std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() {
                std::println("[{}] [{}] \tpromise_type::initial_suspend()", tid(), time());
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                std::println("[{}] [{}] \tpromise_type::final_suspend()", tid(), time());
                return {};
            }

            void return_void() {
                std::println("[{}] [{}] \tpromise_type::return_void()", tid(), time());
            }

            void unhandled_exception() {
                std::println("[{}] [{}] \tpromise_type::unhandled_exception()", tid(), time());
                std::terminate();
            }

            size_t data = 0;
        };

        std::coroutine_handle<promise_type> handle;
    };
}

namespace DefaultTimeout
{
    struct Awaiter
    {
        bool await_ready() noexcept
        {   /** Called immediately before the coroutine is suspended
             *  Allows as such, for some reason, to decide not to suspend after all
             *  Returns true → coroutine is NOT suspended
             *  Typically : return false;
             *  Use case : suspension depends on some data availability
            **/

            std::println("[{}] [{}] Awaiter::await_ready()", tid(), time());

            /** TRUE ==> await_resume() or in case FALSE ==> await_suspend() will be called **/
            return false;
        }

        void await_suspend(std::coroutine_handle<TaskPromise::promise_type> handle) noexcept
        {   /** Called immediately after the coroutine is suspended
             *  Will get called if await_ready() return False
             *  Parameter: the handle of the coroutine that was suspended
             *  In the body you can either return an other coroutine_handle type to change the call execution
             *  Or you ca return nothing
            **/
            std::println("[{}] [{}] Awaiter::await_suspend()", tid(), time());
            std::jthread thread([=]() {
                std::this_thread::sleep_for(std::chrono::seconds(1u));
                handle.promise().data = 1;
                handle.resume();
            });
        }

        void await_resume() noexcept
        {   /** Called when the coroutine is resumed (after a successful suspension)
             *  It is the final result of expression 'co_await ...'
             *  It could return a value or nothing
             *  Can return a value : The value the co_await expression yields
            **/
            std::println("[{}] [{}] Awaiter::await_resume()", tid(), time());
        }
    };

    TaskPromise createCoroutine()
    {
        std::println("[{}] [{}] createCoroutine() step 1", tid(), time());
        co_await Awaiter { };
        std::println("[{}] [{}] createCoroutine() step 2", tid(), time());
    }

    void test()
    {
        std::println("[{}] [{}] main(0)",tid(), time());
        TaskPromise promise = createCoroutine();

        size_t result = promise.handle.promise().data;
        std::println("[{}] [{}] main(1). result = {}", tid(), time(), result);
        promise.handle.resume();

        result = promise.handle.promise().data;
        std::println("[{}] [{}] main(2). result = {}", tid(), time(), result);

        /**
        [139800082312064] [2025-03-02 08:49:38.564162] main(0)
        [139800082312064] [2025-03-02 08:49:38.564215] 	promise_type::get_return_object()
        [139800082312064] [2025-03-02 08:49:38.564219] 	promise_type::initial_suspend()
        [139800082312064] [2025-03-02 08:49:38.564221] main(1). result = 0
        [139800082312064] [2025-03-02 08:49:38.564224] createCoroutine() step 1
        [139800082312064] [2025-03-02 08:49:38.564226] Awaiter::await_ready()
        [139800082312064] [2025-03-02 08:49:38.564228] Awaiter::await_suspend()
        [139800082298624] [2025-03-02 08:49:39.564465] Awaiter::await_resume()
        [139800082298624] [2025-03-02 08:49:39.564592] createCoroutine() step 2
        [139800082298624] [2025-03-02 08:49:39.564601] 	promise_type::return_void()
        [139800082298624] [2025-03-02 08:49:39.564611] 	promise_type::final_suspend()
        [139800082312064] [2025-03-02 08:49:39.564793] main(2). result = 1
        **/
    }
}

namespace TimeoutAsParameter
{
    struct Awaiter
    {
        std::chrono::milliseconds duration;

        explicit Awaiter(const std::chrono::milliseconds d) : duration(d) {
            std::println("[{}] [{}] Awaiter::Awaiter({})", tid(), time(), duration.count());
        }

        bool await_ready() noexcept
        {   /** Called immediately before the coroutine is suspended
             *  Allows as such, for some reason, to decide not to suspend after all
             *  Returns true → coroutine is NOT suspended
             *  Typically : return false;
             *  Use case : suspension depends on some data availability
            **/
            std::println("[{}] [{}] Awaiter::await_ready()", tid(), time());

            /** TRUE ==> await_resume() or in case FALSE ==> await_suspend() will be called **/
            return false;
        }

        void await_suspend(std::coroutine_handle<TaskPromise::promise_type> handle) noexcept
        {   /** Called immediately after the coroutine is suspended
             *  Will get called if await_ready() return False
             *  Parameter: the handle of the coroutine that was suspended
             *  In the body you can either return an other coroutine_handle type to change the call execution
             *  Or you ca return nothing
            **/
            std::println("[{}] [{}] Awaiter::await_suspend()", tid(), time());
            std::jthread thread([=]() {
                std::this_thread::sleep_for(duration);
                handle.promise().data = 1;
                handle.resume();
            });
        }

        void await_resume() noexcept
        {   /** Called when the coroutine is resumed (after a successful suspension)
             *  It is the final result of expression 'co_await ...'
             *  It could return a value or nothing
             *  Can return a value : The value the co_await expression yields
            **/
            std::println("[{}] [{}] Awaiter::await_resume()", tid(), time());
        }
    };

    TaskPromise createCoroutine()
    {
        std::println("[{}] [{}] createCoroutine() step 1", tid(), time());
        co_await Awaiter { std::chrono::seconds(1u) };
        std::println("[{}] [{}] createCoroutine() step 2", tid(), time());
    }

    void test()
    {
        std::println("[{}] [{}] main(0)",tid(), time());
        TaskPromise promise = createCoroutine();

        size_t result = promise.handle.promise().data;
        std::println("[{}] [{}] main(1). result = {}", tid(), time(), result);
        promise.handle.resume();

        result = promise.handle.promise().data;
        std::println("[{}] [{}] main(2). result = {}", tid(), time(), result);

        /**
        [140148684069760] [2025-03-02 08:50:07.049929] main(0)
        [140148684069760] [2025-03-02 08:50:07.049984] 	promise_type::get_return_object()
        [140148684069760] [2025-03-02 08:50:07.049988] 	promise_type::initial_suspend()
        [140148684069760] [2025-03-02 08:50:07.049990] main(1). result = 0
        [140148684069760] [2025-03-02 08:50:07.049993] createCoroutine() step 1
        [140148684069760] [2025-03-02 08:50:07.049995] Awaiter::Awaiter(1000)
        [140148684069760] [2025-03-02 08:50:07.049997] Awaiter::await_ready()
        [140148684069760] [2025-03-02 08:50:07.049999] Awaiter::await_suspend()
        [140148684056320] [2025-03-02 08:50:08.050496] Awaiter::await_resume()
        [140148684056320] [2025-03-02 08:50:08.050617] createCoroutine() step 2
        [140148684056320] [2025-03-02 08:50:08.050627] 	promise_type::return_void()
        [140148684056320] [2025-03-02 08:50:08.050637] 	promise_type::final_suspend()
        [140148684069760] [2025-03-02 08:50:08.050799] main(2). result = 1
        **/
    }
}

void StdCoroutines::Simple::Waitable_Coroutine_2::TestAll()
{
    // DefaultTimeout::test();
    TimeoutAsParameter::test();
}
