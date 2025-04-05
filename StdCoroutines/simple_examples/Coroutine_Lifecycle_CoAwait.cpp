/**============================================================================
Name        : Coroutine_Lifecycle_CoAwait.cpp
Created on  : 28.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine lifecycle demo
============================================================================**/

#include "SimpleCoroutines.h"

#include <coroutine>
#include <print>

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

**/

namespace
{
    struct [[nodiscard]] MyCoroutine
    {
        struct promise_type
        {
            MyCoroutine get_return_object()
            {
                std::println("0. get_return_object");

                /// This method is called when the coroutine is first created. It typically returns a handle to the
                /// coroutine itself, which can be used by the caller to control and monitor the coroutine’s execution.
                /// The handle might allow resuming, destroying, or retrieving the result from the coroutine.

                return {};
            }

            std::suspend_never initial_suspend()
            {
                std::println("1. initial_suspend");

                /// The initial_suspend() method decides whether the coroutine should start running immediately or if it
                /// should wait until an explicit resume call is made. Similarly, the final_suspend method determines
                /// if the coroutine should perform any final suspension steps before completing.
                ///
                /// initial_suspend() can return either
                ///     std::suspend_always --> which means the coroutine will suspend before doing anything
                ///     std::suspend_never  --> which means it will start executing immediately.
                ///

                return {};
            }

            std::suspend_never final_suspend() noexcept
            {
                std::println("3. final_suspend");

                /// final_suspend() often returns std::suspend_always to make sure that the coroutine cleans
                /// up its resources properly before exiting.

                return {};
            }

            void return_void()
            {
                std::println("2. return_void");

                /// Depending on the coroutine’s design, it may or may not return a value.
                /// The return_void method is used when the coroutine doesn't return anything, while return_value is
                /// used when the coroutine returns a specific value.
            }

            void unhandled_exception()
            {
                std::println("4. unhandled_exception");

                std::terminate();

                /// If an exception occurs inside a coroutine and isn’t caught, the unhandled_exception method is invoked.
                /// This method usually terminates the program or performs some error handling, depending on the
                /// specific implementation.
            }
        };

        MyCoroutine() {
            std::println("MyCoroutine()");
        }

        ~MyCoroutine() {
            std::println("~MyCoroutine()");
        }
    };

    MyCoroutine myCoroutineFunction()
    {
        std::println("--> About to suspend this coroutine");
        co_await std::suspend_always{};
        std::println("--> this won't be printed until after we resume");
        co_return;
    }
}


/**
C++ provides two keywords you can use to suspend a coroutine:
-  co_await   (co_await is the more basic one)
-  co_yield.  (co_yield is defined in terms of it)

The general idea is that, whatever operand you provide to the co_await keyword,
the compiler has to convert it into an awaiter object that will tell it how to manage the suspension.

*/

void StdCoroutines::Simple::Coroutine_Lifecycle_CoAwait::TestAll()
{
    MyCoroutine er = myCoroutineFunction();
}
