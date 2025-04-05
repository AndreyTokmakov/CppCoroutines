/**============================================================================
Name        : Awaiter_and_Awaitable.cpp
Created on  : 20.03.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "SimpleCoroutines.h"
#include "Utilities.h"

#include <chrono>
#include <thread>
#include <random>
#include <vector>

#include <coroutine>
#include <print>
#include <source_location>

namespace
{
    auto tid() { return std::this_thread::get_id(); }
    auto time() { return Utilities::getCurrentTime(); }

}

namespace
{
    struct SimpleCoroutine
    {
        struct promise_type
        {
            SimpleCoroutine get_return_object() {
                return SimpleCoroutine { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_never initial_suspend() const noexcept  {
                return {};
            }

            std::suspend_always final_suspend() const noexcept  {
                return {};
            }

            void unhandled_exception() const { }
            void return_void() const noexcept { }
        };

        void resume() const {
            handle.resume();
        }

        ~SimpleCoroutine() {
            if (handle)
                handle.destroy();
        }

        std::coroutine_handle<promise_type> handle;
    };
}

namespace Awaitable_Demo
{   /**
    Awaitable
        A type that supports the "co_await" operator is called an Awaitable type.
        C++20 introduced a new unary operator "co_await" that can be applied to an expression.
    **/

    // Awaitable
    struct DummyAwaitable
    {
        std::suspend_always operator co_await() const
        {
            return {};
        }
    };

    SimpleCoroutine print_hello_world()
    {
        std::print("Hello ");
        co_await DummyAwaitable {};
        std::println("world ");
    }

    void demo()
    {
        const SimpleCoroutine coroutine = print_hello_world();
        coroutine.resume();
    }
}


namespace Awaiter_Demo
{
    /**
    * An Awaiter type is a type that implements the three special methods that are called as part of a co_await expression:
    * - await_ready()
    * - await_suspend()
    * - await_resume().
    *
    * For example, standard library defined trivial awaiters i.e.
    * - std::suspend_always-
    * - std::suspend_never.
    *
    * Note that a type can be both an Awaitable type and an Awaiter type.
    */
    struct DummyAwaiter
    {
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<>) {}
        void await_resume() {}
    };

    SimpleCoroutine print_hello_world()
    {
        std::print("Hello ");
        co_await DummyAwaiter {};
        std::println("world ");
    }

    void demo()
    {
        const SimpleCoroutine coroutine = print_hello_world();
        coroutine.resume();
    }
}

void StdCoroutines::Simple::Awaiter_and_Awaitable::TestAll()
{
    // Awaitable_Demo::demo();
    Awaiter_Demo::demo();
}
