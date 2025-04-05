/**============================================================================
Name        : Yield_Coroutine.cpp
Created on  : 27.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Yield_Coroutine.h
============================================================================**/

#include "SimpleCoroutines.h"
#include "Utilities.h"

#include <coroutine>
#include <print>
#include <chrono>
#include <thread>

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

namespace
{
    using Utilities::getCurrentTime;
}

namespace
{
    using namespace std::string_literals;

    struct MyCoroutineTask
    {
        struct promise_type
        {
            std::string output_data { };

            MyCoroutineTask get_return_object() noexcept {
                std::println("[{}] \tpromise_type::get_return_object()", getCurrentTime());
                return MyCoroutineTask {  *this };
            }

            void return_void() noexcept {
                std::println("[{}] \tpromise_type::return_void()", getCurrentTime());
            }

            /// Will be called from instruction 'co_yield "Hello from the coroutine\n"s;'
            std::suspend_always yield_value(std::string msg) noexcept {
                std::println("[{}] \tpromise_type::yield_value('{}') ==> std::suspend_always()", getCurrentTime(), msg);
                output_data = std::move(msg);
                return {};
            }

            std::suspend_always initial_suspend() noexcept {
                std::println("[{}] \tpromise_type::initial_suspend() ==> std::suspend_always()", getCurrentTime());
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                std::println("[{}] \tpromise_type::final_suspend() ==> std::suspend_always()", getCurrentTime());
                return {};
            }

            void unhandled_exception() noexcept {
                std::println("[{}] \tpromise_type::unhandled_exception()", getCurrentTime());
            }
        };

        std::coroutine_handle<promise_type> handle{};

        explicit MyCoroutineTask(promise_type& promise) :
                handle { std::coroutine_handle<promise_type>::from_promise(promise)}
        {
            std::println("[{}] ReturnType::ReturnType()", getCurrentTime());
        }

        ~MyCoroutineTask() noexcept
        {
            if (handle) {
                handle.destroy();
            }
            std::println("[{}] ReturnType::~ReturnType()", getCurrentTime());
        }

        [[nodiscard]]
        std::string getValue() const noexcept
        {
            std::println("[{}] ReturnType::getValue()", getCurrentTime());
            if (!handle.done()) {
                handle.resume();
            }
            return std::move(handle.promise().output_data);
        }
    };

    MyCoroutineTask createCoroutine()
    {
        std::println("[{}] createCoroutine() entered", getCurrentTime());

        co_yield "Hello from the coroutine"s;

        std::println("[{}] createCoroutine() after co_yield ..", getCurrentTime());

        co_return;
    }

};


void StdCoroutines::Simple::Yield_Coroutine::TestAll()
{
    MyCoroutineTask coro = createCoroutine();
    std::println("[{}] main function()", getCurrentTime());
    const std::string returnedValue = coro.getValue();

    std::println("[{}] main returned value: {}", getCurrentTime(),returnedValue);
}

/**
[2025-03-09 16:38:06.107840] 	promise_type::get_return_object()
[2025-03-09 16:38:06.107899] ReturnType::ReturnType()
[2025-03-09 16:38:06.107902] 	promise_type::initial_suspend() ==> std::suspend_always()
[2025-03-09 16:38:06.107905] main function()
[2025-03-09 16:38:06.107907] ReturnType::getValue()
[2025-03-09 16:38:06.107908] createCoroutine() entered
[2025-03-09 16:38:06.107910] 	promise_type::yield_value('Hello from the coroutine') ==> std::suspend_always()
[2025-03-09 16:38:06.107913] main returned value: Hello from the coroutine
[2025-03-09 16:38:06.107917] ReturnType::~ReturnType()
*/