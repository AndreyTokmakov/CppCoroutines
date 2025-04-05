/**============================================================================
Name        : Yield_Coroutine_Values_from_List.cpp
Created on  : 09.03.2025
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
    auto tid() { return std::this_thread::get_id();}
    auto time() { return Utilities::getCurrentTime();}

    void log(const std::string_view message,
             const std::source_location location = std::source_location::current())
    {
        std::println("File: {} [{}:{}] {} : {}",
                     location.file_name(), location.line(), location.column(), location.function_name(),
                     message);
    }
}

namespace
{
    std::random_device randomDevice{};
    std::mt19937 generator(randomDevice());

    const std::vector<std::string> repository
    {
        "Value_1",
        "Value_2",
        "Value_3",
        "Value_4",
        "Value_5",
        "Value_6",
        "Value_7",
        "Value_8",
        "Value_9",
        "Value_10"
    };
}

namespace
{
    using namespace std::string_literals;

    struct CoroutineTask
    {
        struct promise_type
        {
            CoroutineTask get_return_object() {
                return CoroutineTask {std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_always initial_suspend() {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void unhandled_exception() {
                std::terminate();
            }

            std::suspend_always yield_value(const std::string& valueIn)
            {
                value = valueIn;
                return std::suspend_always{};
            }

            std::string value;
            // uint32_t counter { 0 };
        };

        explicit CoroutineTask(const std::coroutine_handle<promise_type>& handle) : coroHandle { handle } {
        }

        ~CoroutineTask()
        {
            if (coroHandle) {
                coroHandle.destroy();
            }
        }

        CoroutineTask(const CoroutineTask&) = delete;
        CoroutineTask& operator=(const CoroutineTask&) = delete;

        [[nodiscard]]
        bool hasNext() const
        {
            if (!coroHandle || coroHandle.done()) {
                return false; // we are done
            }

            coroHandle.resume();
            return !coroHandle.done();
        }

        [[nodiscard]]
        std::string value() const
        {
            return coroHandle.promise().value;
        }

    private:
        std::coroutine_handle<promise_type> coroHandle;
    };

    CoroutineTask createFetcher(int size)
    {
        std::uniform_int_distribution<uint32_t> distribution(0, repository.size() - 1);
        for (int i = 0; i < size; ++i)
        {
            co_yield repository[distribution(generator)];
        }
    }

};


void StdCoroutines::Simple::Yield_Coroutine_Values_from_List::TestAll()
{
    const CoroutineTask fetcher = createFetcher(7);
    while (fetcher.hasNext()) {
        std::println("value = {}", fetcher.value());
    }
}
