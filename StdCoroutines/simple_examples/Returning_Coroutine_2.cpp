/**============================================================================
Name        : Returning_Coroutine_2.cpp
Created on  : 01.03.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine returning value
============================================================================**/

#include "SimpleCoroutines.h"

#include <coroutine>
#include <print>

namespace
{
    struct ReturnValueCoroutine
    {
        struct promise_type
        {
            int value;

            ReturnValueCoroutine get_return_object()
            {
                std::println("\t0. get_return_object");
                return ReturnValueCoroutine { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_never initial_suspend()
            {
                std::println("\t1. initial_suspend");
                return {};
            }

            std::suspend_never final_suspend() noexcept
            {
                std::println("\t4. final_suspend");
                return {};
            }

            void return_value(const int v)
            {
                std::println("\t3. return_value ({})", v);
                value = v;
            }

            void unhandled_exception()
            {
                std::terminate();
            }
        };

        std::coroutine_handle<promise_type> coroHandle;

        explicit ReturnValueCoroutine(const std::coroutine_handle<promise_type> handle) : coroHandle { handle } {
            std::println("ReturnValueCoroutine()");
        }

        ~ReturnValueCoroutine() {
            std::println("~ReturnValueCoroutine()");
        }

        [[nodiscard]]
        int get() const {
            return coroHandle.promise().value;
        }
    };

    ReturnValueCoroutine computeValue() {
        co_return 12345;
    }
}

/// In this example, the coroutine computeValue returns an integer value using co_return.
/// The promise type manages the returned value, and the get_return_object method returns a ReturnValue
/// object that the caller can use to retrieve the result.

void StdCoroutines::Simple::Returning_Coroutine_2::TestAll()
{
    std::println("Start of coroutine");
    const ReturnValueCoroutine result = computeValue();
    std::println("Computed Value: {}\nDone", result.get());
}
