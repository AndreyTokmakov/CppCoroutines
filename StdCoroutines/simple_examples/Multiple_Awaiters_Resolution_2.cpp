/**============================================================================
Name        : Multiple_Awaiters_Resolution_2.cpp
Created on  : 28.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine Waitable
============================================================================**/

#include "SimpleCoroutines.h"
#include <iostream>
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

    struct Event
    {
        std::chrono::milliseconds duration;
    };

    // TODO: Concepts ??? to check CoroType::promise_type
    template<typename CoroType>
    struct DurationAwaiter
    {
        std::chrono::milliseconds duration {};

        explicit DurationAwaiter(const std::chrono::milliseconds d) : duration(d)
        {
            std::println("[{}] [{}] DurationAwaiter::DurationAwaiter({})", tid(), time(), duration.count());
        }

        bool await_ready() noexcept
        {
            std::println("[{}] [{}] DurationAwaiter::await_ready()", tid(), time());
            return false;
        }

        void await_suspend(std::coroutine_handle<typename CoroType::promise_type> hInputCoro) noexcept
        {
            std::println("[{}] [{}] DurationAwaiter::await_suspend()", tid(), time());
            std::jthread thread([&hInputCoro, this]() {
                std::this_thread::sleep_for(duration);
                hInputCoro.promise().data = 1;
                hInputCoro.resume();
            });
        }

        void await_resume() noexcept {
            std::println("[{}] [{}] DurationAwaiter::await_resume()", tid(), time());
        }
    };

    // TODO: Concepts ??? to check CoroType::promise_type
    template<typename CoroType>
    struct EventAwaiter
    {
        Event event {};

        explicit EventAwaiter(const Event ent): event { ent } {
            std::println("[{}] [{}] EventAwaiter::EventAwaiter({})", tid(), time(), event.duration.count());
        }

        bool await_ready() noexcept
        {
            std::println("[{}] [{}] EventAwaiter::await_ready()", tid(), time());
            return false;
        }

        void await_suspend(std::coroutine_handle<typename CoroType::promise_type> hInputCoro) noexcept
        {
            std::println("[{}] [{}] EventAwaiter::await_suspend()", tid(), time());
            std::jthread thread([&hInputCoro, this]() {
                std::this_thread::sleep_for(event.duration);
                hInputCoro.promise().data = 1;
                hInputCoro.resume();
            });
        }

        void await_resume() noexcept {
            std::println("[{}] [{}] EventAwaiter::await_resume()", tid(), time());
        }
    };
}

namespace
{
    struct TaskPromise
    {
        struct promise_type
        {
            int data { 0 };

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

            DurationAwaiter<TaskPromise> await_transform(const std::chrono::milliseconds timeout) noexcept {
                return DurationAwaiter<TaskPromise>(timeout);
            }

            EventAwaiter<TaskPromise> await_transform(const Event& event) noexcept {
                return EventAwaiter<TaskPromise>(event);
            }
        };

        std::coroutine_handle<promise_type> handle;
    };
}

namespace
{
    TaskPromise createCoroutine(auto timeoutEvent)
    {
        std::println("[{}] [{}] createCoroutine() step 1", tid(), time());
        co_await timeoutEvent;
        std::println("[{}] [{}] createCoroutine() step 2", tid(), time());
    }

    void test(auto timeoutEvent)
    {
        std::println("[{}] [{}] main(0)",tid(), time());
        TaskPromise promise = createCoroutine(timeoutEvent );

        size_t result = promise.handle.promise().data;
        std::println("[{}] [{}] main(1). result = {}", tid(), time(), result);
        promise.handle.resume();

        result = promise.handle.promise().data;
        std::println("[{}] [{}] main(2). result = {}", tid(), time(), result);
    }
}

/**
1. Depending of input parameter type
2. Different TaskPromise::promise_type::await_transform(...) will be called
3. Different types of Awaitable-s will be instantiated:
   - DurationAwaiter<TaskPromise>
   - EventAwaiter<TaskPromise>
**/


// DurationAwaiter constryctor have no 'CoroType::promise_type& promise' member

void StdCoroutines::Simple::Multiple_Awaiters_Resolution_2::TestAll()
{
    /** Will use DurationAwaiter **/
    test(std::chrono::seconds(1u));

    std::cout << std::string(180,'=') << std::endl;

    /** Will use EventAwaiter **/
    test(Event {std::chrono::seconds(1u)});
}

/**
[139961643191168] [2025-03-02 14:29:49.833525] main(0)
[139961643191168] [2025-03-02 14:29:49.833582] 	promise_type::get_return_object()
[139961643191168] [2025-03-02 14:29:49.833585] 	promise_type::initial_suspend()
[139961643191168] [2025-03-02 14:29:49.833587] main(1). result = 0
[139961643191168] [2025-03-02 14:29:49.833590] createCoroutine() step 1
[139961643191168] [2025-03-02 14:29:49.833592] DurationAwaiter::DurationAwaiter(1000)
[139961643191168] [2025-03-02 14:29:49.833595] DurationAwaiter::await_ready()
[139961643191168] [2025-03-02 14:29:49.833597] DurationAwaiter::await_suspend()
[139961643177728] [2025-03-02 14:29:50.834078] DurationAwaiter::await_resume()
[139961643177728] [2025-03-02 14:29:50.834217] createCoroutine() step 2
[139961643177728] [2025-03-02 14:29:50.834227] 	promise_type::return_void()
[139961643177728] [2025-03-02 14:29:50.834236] 	promise_type::final_suspend()
[139961643191168] [2025-03-02 14:29:50.834380] main(2). result = 1
====================================================================================================================================================================================
[139961643191168] [2025-03-02 14:29:50.834517] main(0)
[139961643191168] [2025-03-02 14:29:50.834531] 	promise_type::get_return_object()
[139961643191168] [2025-03-02 14:29:50.834543] 	promise_type::initial_suspend()
[139961643191168] [2025-03-02 14:29:50.834556] main(1). result = 0
[139961643191168] [2025-03-02 14:29:50.834567] createCoroutine() step 1
[139961643191168] [2025-03-02 14:29:50.834578] EventAwaiter::EventAwaiter(3000)
[139961643191168] [2025-03-02 14:29:50.834595] EventAwaiter::await_ready()
[139961643191168] [2025-03-02 14:29:50.834604] EventAwaiter::await_suspend()
[139961643177728] [2025-03-02 14:29:53.835200] EventAwaiter::await_resume()
[139961643177728] [2025-03-02 14:29:53.835265] createCoroutine() step 2
[139961643177728] [2025-03-02 14:29:53.835276] 	promise_type::return_void()
[139961643177728] [2025-03-02 14:29:53.835286] 	promise_type::final_suspend()
[139961643191168] [2025-03-02 14:29:53.835431] main(2). result = 1
**/