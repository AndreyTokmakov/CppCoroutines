/**============================================================================
Name        : Mutiple_Awaiters_Initiail_Final.cpp
Created on  : 05.04.2025
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

    template<typename CoroType>
    struct EventAwaiter
    {
        Event event {};

        explicit EventAwaiter(const Event ent): event { ent } {
            std::println("[{}] [{}] \t\tEventAwaiter::EventAwaiter({})", tid(), time(), event.duration.count());
        }

        bool await_ready() noexcept
        {
            std::println("[{}] [{}] \t\tEventAwaiter::await_ready()", tid(), time());

            /** TRUE ==> await_resume() or in case FALSE ==> await_suspend() will be called **/
            return false;
        }

        void await_suspend(std::coroutine_handle<typename CoroType::promise_type> hInputCoro) noexcept
        {
            std::println("[{}] [{}] \t\tEventAwaiter::await_suspend()", tid(), time());
            std::jthread thread([&hInputCoro, this]() {
                std::this_thread::sleep_for(event.duration);
                hInputCoro.promise().data = 1;
                hInputCoro.resume();
            });
        }

        void await_resume() noexcept {
            std::println("[{}] [{}] \t\tEventAwaiter::await_resume()", tid(), time());
        }
    };

    /** Same as std::suspend_always **/
    struct InitialAwaiter
    {
        InitialAwaiter(){
            std::println("[{}] [{}] \t\tInitialAwaiter::InitialAwaiter()", tid(), time());
        }

        ~InitialAwaiter() {
            std::println("[{}] [{}] \t\tInitialAwaiter::~InitialAwaiter()", tid(), time());
        }

        bool await_ready() const noexcept {
            /** TRUE ==> await_resume() or in case FALSE ==> await_suspend() will be called **/
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept {
            std::println("[{}] [{}] \t\tInitialAwaiter::await_suspend()", tid(), time());
        }

        void await_resume() const noexcept { }
    };

    /** Same as std::suspend_always **/
    struct FinalAwaiter
    {
        FinalAwaiter() {
            std::println("[{}] [{}] \t\tFinalAwaiter::FinalAwaiter()", tid(), time());
        }

        ~FinalAwaiter() {
            std::println("[{}] [{}] \t\tFinalAwaiter::~FinalAwaiter()", tid(), time());
        }

        bool await_ready() const noexcept {
            /** TRUE ==> await_resume() or in case FALSE ==> await_suspend() will be called **/
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept {
            std::println("[{}] [{}] \t\tFinalAwaiter::await_suspend()", tid(), time());
        }

        void await_resume() const noexcept { }
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

            InitialAwaiter initial_suspend() {
                std::println("[{}] [{}] \tpromise_type::initial_suspend()", tid(), time());
                return {};
            }

            FinalAwaiter final_suspend() noexcept {
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
}

/**
1. Depending of input parameter type
2. Different TaskPromise::promise_type::await_transform(...) will be called
3. Different types of Awaitable-s will be instantiated:
   - DurationAwaiter<TaskPromise>
   - EventAwaiter<TaskPromise>
**/



void StdCoroutines::Simple::Mutiple_Awaiters_Initiail_Final::TestAll()
{

    std::println("[{}] [{}] main(0)",tid(), time());
    TaskPromise promise = createCoroutine(Event {std::chrono::seconds(1u)} );

    size_t result = promise.handle.promise().data;
    std::println("[{}] [{}] main(1). result = {}", tid(), time(), result);
    promise.handle.resume();

    result = promise.handle.promise().data;
    std::println("[{}] [{}] main(2). result = {}", tid(), time(), result);
}

/**
[140026242124672] [2025-04-05 17:45:47.331779] main(0)
[140026242124672] [2025-04-05 17:45:47.331839] 	promise_type::get_return_object()
[140026242124672] [2025-04-05 17:45:47.331843] 	promise_type::initial_suspend()
[140026242124672] [2025-04-05 17:45:47.331845] 		InitialAwaiter::InitialAwaiter()
[140026242124672] [2025-04-05 17:45:47.331848] 		InitialAwaiter::await_suspend()
[140026242124672] [2025-04-05 17:45:47.331850] main(1). result = 0
[140026242124672] [2025-04-05 17:45:47.331853] 		InitialAwaiter::~InitialAwaiter()
[140026242124672] [2025-04-05 17:45:47.331856] createCoroutine() step 1
[140026242124672] [2025-04-05 17:45:47.331858] 		EventAwaiter::EventAwaiter(1000)
[140026242124672] [2025-04-05 17:45:47.331863] 		EventAwaiter::await_ready()
[140026242124672] [2025-04-05 17:45:47.331866] 		EventAwaiter::await_suspend()
[140026242111232] [2025-04-05 17:45:48.332065] 		EventAwaiter::await_resume()
[140026242111232] [2025-04-05 17:45:48.332179] createCoroutine() step 2
[140026242111232] [2025-04-05 17:45:48.332189] 	promise_type::return_void()
[140026242111232] [2025-04-05 17:45:48.332198] 	promise_type::final_suspend()
[140026242111232] [2025-04-05 17:45:48.332207] 		FinalAwaiter::FinalAwaiter()
[140026242111232] [2025-04-05 17:45:48.332216] 		FinalAwaiter::await_suspend()
[140026242124672] [2025-04-05 17:45:48.332375] main(2). result = 1
**/