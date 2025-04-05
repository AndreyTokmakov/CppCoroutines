/**============================================================================
Name        : Returning_Coroutine.cpp
Created on  : 28.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine returning value
============================================================================**/

#include "SimpleCoroutines.h"

/**
 * How to make Coroutine to return Non-Void value:
 *  --> Need to implement 'return_value(auto val)' method
 *
 * In this example, the coroutine doesn’t return a value at all (the co_return statement has no parameter).
 * So I’ve filled in a method called 'return_void()' in the promise type, which will be called when co_return
 * is executed without a value, or when code falls off the end of the coroutine.
 *
 * If we want for coroutine to have a non-void return value, then we need to define and implements method called 'return_value()'
 * taking one argument, which would have been called when the coroutine executed co_return with a return-value expression.
 */


namespace
{
    struct ReturnValue
    {
        struct promise_type
        {
            int value { 0 };

            ReturnValue get_return_object() {
                return ReturnValue { this };
            }

            std::suspend_never initial_suspend() {
                return {};
            }

            std::suspend_never final_suspend() noexcept {
                return {};
            }

            void return_value(const int val) {
                value = val;
            }

            void unhandled_exception() {
                std::terminate();
            }
        };

        promise_type* promise;

        explicit ReturnValue(promise_type* p) : promise(p) {
        }

        [[nodiscard]]
        int get() const {
            return promise->value;
        }
    };

    ReturnValue computeValue() {
        co_return 42;
    }
}

/// In this example, the coroutine computeValue returns an integer value using co_return.
/// The promise type manages the returned value, and the get_return_object method returns a ReturnValue
/// object that the caller can use to retrieve the result.

void StdCoroutines::Simple::Returning_Coroutine::TestAll()
{
    std::println("Start of coroutine");
    ReturnValue result = computeValue();
    std::println("Computed Value: {}\nDone", result.get() );
}
