/**============================================================================
Name        : Waitable_Coroutine.cpp
Created on  : 28.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine Waitable
============================================================================**/

#include "SimpleCoroutines.h"

#include <chrono>
#include <thread>

namespace {
    using Utilities::getCurrentTime;
}

namespace
{
    /**
    Coroutine Functions and Suspension Points
    A coroutine function is a special type of function in C++ that can be suspended and resumed at specific points
    during its execution.
    This is achieved using three key keywords: co_await, co_yield, and co_return.

    1. co_await:  This keyword is used to suspend the execution of the coroutine until a particular
                  condition is met or an asynchronous operation is completed.

                  Унарный оператор, позволяющий, в общем случае, приостановить выполнение сопрограммы и
                  передать управление вызывающей стороне, пока не завершатся вычисления представленные операндом;

    2. co_yield:  This keyword allows the coroutine to produce a value and suspend its execution.
                  It can be resumed later, continuing from the point after co_yield.

                  Унарный оператор, частный случай оператора co_await, позволяющий приостановить выполнение сопрограммы
                  и передать управление и значение операнда вызывающей стороне;

    3. co_return: This keyword is used to return a value from the coroutine and finalize its execution.

                  Оператор завершает работу сопрограммы, возвращая значение, после вызова сопрограмма
                  больше не сможет возобновить свое выполнение.


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

    struct AwaiterTimer
    {
        std::chrono::milliseconds duration;

        explicit AwaiterTimer(const std::chrono::milliseconds d) : duration(d) {
            std::println("[{}] AwaiterTimer::AwaiterTimer({})", getCurrentTime(), duration.count());
        }

        [[nodiscard]]
        bool await_ready() noexcept
        {
            /** Called immediately before the coroutine is suspended
             *  Allows as such, for some reason, to decide not to suspend after all
             *  Returns true → coroutine is NOT suspended
             *  Typically : return false;
             *  Use case : suspension depends on some data availability
            **/
            std::println("[{}] AwaiterTimer::await_ready()", getCurrentTime());

            /** TRUE ==> await_resume() or in case FALSE ==> await_suspend() will be called **/
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            /** Called immediately after the coroutine is suspended
             *  Will get called if await_ready() return False
             *  Parameter: the handle of the coroutine that was suspended
             *  In the body you can either return an other coroutine_handle type to change the call execution
             *  Or you ca return nothing
            **/
            std::println("[{}] Timer::await_suspend() entered", getCurrentTime());
            std::thread([handle, this]() {
                std::this_thread::sleep_for(duration);
                handle.resume();
            }).detach();
        }

        void await_resume() const
        {   /** Called when the coroutine is resumed (after a successful suspension)
             *  It is the final result of expression 'co_await ...'
             *  It could return a value or nothing
             *  Can return a value : The value the co_await expression yields
            **/
            std::println("[{}] Timer::await_resume()", getCurrentTime());
        }
    };

    struct MyCoroutineTask
    {
        struct promise_type
        {
            MyCoroutineTask get_return_object() {
                std::println("[{}] get_return_object", getCurrentTime());
                return MyCoroutineTask{};
            }

            std::suspend_never initial_suspend() {
                std::println("[{}] initial_suspend", getCurrentTime());
                return {};
            }

            std::suspend_never final_suspend() noexcept {
                std::println("[{}] initial_suspend", getCurrentTime());
                return {};
            }

            void return_void() {
                std::println("[{}] return_void", getCurrentTime());
            }

            void unhandled_exception() {
                std::terminate();
            }
        };

        MyCoroutineTask run()
        {
            std::println("[{}] Starting timer... ", getCurrentTime());
            co_await AwaiterTimer { std::chrono::seconds(2u) };
            std::println("[{}] Starting timer...", getCurrentTime());
            std::println("[{}] Timer finished.", getCurrentTime());
        }

        MyCoroutineTask() {
            std::println("[{}] MyCoroutineTask()", getCurrentTime());
        }

        ~MyCoroutineTask() {
            std::println("[{}] ~MyCoroutineTask()", getCurrentTime());
        }
    };
}


void StdCoroutines::Simple::Waitable_Coroutine::TestAll()
{
    MyCoroutineTask task;
    task.run();
    std::this_thread::sleep_for(std::chrono::seconds(3u));
}
