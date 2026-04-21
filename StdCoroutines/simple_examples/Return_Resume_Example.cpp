/**============================================================================
Name        : Return_Resume_Example.cpp
Created on  :
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Return Resume Lifecycle
============================================================================**/

#include "SimpleCoroutines.hpp"
#include <iostream>
#include <chrono>
#include <optional>


namespace
{
    struct ComputeResult
    {
        struct promise_type
        {
            std::optional<int> storedValue { std::nullopt };

            [[nodiscard]]
            ComputeResult get_return_object() noexcept {
                return ComputeResult { *this };
            }

            std::suspend_always initial_suspend() {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void return_value(int value) {
                std::cout << "promise_type::return_value(): " << storedValue.value() << std::endl;
                storedValue = value;
            }
            void unhandled_exception() {
                storedValue = std::nullopt;
            }
        };

        std::coroutine_handle<promise_type> handle;

        explicit ComputeResult(promise_type& promise) :
            handle { std::coroutine_handle<promise_type>::from_promise(promise) } {
        }

        ~ComputeResult()
        {
            if (handle) {
                handle.destroy();
            }
        }

        ComputeResult(ComputeResult&& other) noexcept : handle(other.handle) {
            other.handle = nullptr;
        }

        void run()
        {
            while (!handle.done()) {
                handle.resume();
            }
        }

        [[nodiscard]]
        std::optional<int> getResult() const {
            return handle.promise().storedValue;
        }
    };

    ComputeResult computeSum(int n)
    {
        int sum = 0;
        for (int i = 1; i <= n; ++i) {
            sum += i;
            co_await std::suspend_always{};  // yield control periodically
        }
        co_return sum;
    }

    void demo()
    {
        ComputeResult computation = computeSum(5);
        computation.run();

        if (const auto result = computation.getResult()) {
            std::cout << "Result: " << *result << std::endl;
        }
    }
}

void StdCoroutines::Simple::Return_Resume_Example::TestAll()
{
    demo();
    // Result: 15
}
