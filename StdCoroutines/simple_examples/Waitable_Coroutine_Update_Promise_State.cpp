/**============================================================================
Name        : Yield_Coroutine_Values_from_List.cpp
Created on  : 09.03.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "SimpleCoroutines.h"

#include <iostream>
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

3. await_resume() : Defines what happens when the coroutine is resumed
                    Often returning a value or performing some final action before execution continues.
**/

namespace
{
    auto tid() { return std::this_thread::get_id();}
    auto time() { return Utilities::getCurrentTime();}
}

namespace
{
    using namespace std::string_literals;

    template<typename CoroType>
    struct Awaiter
    {
        CoroType::promise_type& promise;

        /** await_ready():
         *  This is called to check whether the coroutine is suspended.
         *  If that is the case,it returns false.
         *  In our example, it always returns true to indicate the coroutine is not suspended.
        **/
        [[nodiscard]]
        bool await_ready() const noexcept
        {
            std::println("[{}] [{}] Awaiter::await_ready()", tid(), time());
            return true;
        }

        /** await_suspend():
         *  In our simple awaiter, this returns void, meaning the control is passed to the caller and the coroutine is suspended.
         *  It’s also possible for await_suspend to return a Boolean.
         *  Returning true in this case is like returning void.
         *  Returning false means the coroutine is resumed.
        **/
        void await_suspend(std::coroutine_handle<typename CoroType::promise_type> ) const noexcept
        {
            std::println("[{}] [{}] DurationAwaiter::await_suspend()", tid(), time());
        }

        /** await_resume():
         *  This resumes the coroutine and generates the result of the co_await expression.
         **/
        [[nodiscard]]
        std::string await_resume() const noexcept
        {
            std::println("[{}] [{}] DurationAwaiter::await_resume()", tid(), time());
            return std::move(promise.input_data);
        }
    };


    struct CoroutinesTask
    {
        struct promise_type
        {
            std::string input_data { };

            CoroutinesTask get_return_object() noexcept
            {
                // std::println("[{}] [{}] \tpromise_type::get_return_object()", tid(), time());
                return CoroutinesTask {std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            void return_void() noexcept {
                std::println("[{}] [{}] \t\tpromise_type::return_void()", tid(), time());
            }

            std::suspend_always initial_suspend() noexcept
            {
                std::println("[{}] [{}] \t\tpromise_type::initial_suspend()", tid(), time());
                return {};
            }

            std::suspend_always final_suspend() noexcept
            {
                std::println("[{}] [{}] \t\tpromise_type::final_suspend()", tid(), time());
                return {};
            }

            void unhandled_exception() {
                std::println("[{}] [{}] \t\tpromise_type::unhandled_exception()", tid(), time());
                std::terminate();
            }

            /**
             * when the coroutine calls co_await, the compiler will generate code to call a function in the promise
             * object called await_transform(), which has a parameter of the same type as the data the coroutine is waiting for.
             *
             * As its name implies, await_transform is a function that transforms any object (in our example, std::string)
             * into an awaitable object.
             *
             * std::string is not awaitable, hence the previous compiler error. await_transform() must return an awaiter object.
             * his is just a simple struct implementing a required interface for the awaiter to be usable by the compiler.
            **/

            auto await_transform(const std::string&) noexcept
            {
                std::println("[{}] [{}] CoroutinesTask::promise_type::await_transform()", tid(), time());
                return Awaiter<CoroutinesTask>(*this);
            }
        };

        std::coroutine_handle<promise_type> handle {};

        explicit CoroutinesTask(const std::coroutine_handle<promise_type>& handle) : handle { handle }
        {
            std::println("[{}] [{}] CoroutinesTask::CoroutinesTask()", tid(), time());
        }

        ~CoroutinesTask() noexcept
        {
            if (handle) {
                handle.destroy();
            }
            std::println("[{}] [{}] CoroutinesTask::~CoroutinesTask()", tid(), time());
        }

        void putValue(std::string msg) const noexcept
        {
            handle.promise().input_data = std::move(msg);
            if (!handle.done()) {
                handle.resume();
            }
        }
    };

    CoroutinesTask createCoroutine()
    {
        std::println("[{}] [{}] createCoroutine(1)", tid(), time());

        std::cout << co_await std::string{ };

        std::println("[{}] [{}] createCoroutine(2)", tid(), time());

        co_return;
    }
};


void StdCoroutines::Simple::Waitable_Coroutine_Update_Promise_State::TestAll()
{
    CoroutinesTask task = createCoroutine();
    task.putValue("Hello from main\n"s);
}
