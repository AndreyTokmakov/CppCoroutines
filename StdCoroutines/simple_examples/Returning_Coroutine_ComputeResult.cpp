/**============================================================================
Name        : Returning_Coroutine_ComputeResult.cpp
Created on  : 22.05.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "SimpleCoroutines.hpp"


#include <iostream>
#include <syncstream>
#include <utility>
#include <thread>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime()  \
<< " [" << std::this_thread::get_id() << "] "

namespace
{
    struct ComputeResult
    {
        struct Promise;
        using promise_type = Promise;

        struct Promise
        {
            ComputeResult get_return_object()  {
                return ComputeResult {
                    std::coroutine_handle<Promise>::from_promise(*this)
                };
            }

            // Lazy: don't start until explicitly resumed
            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            // Stay alive so we can read the result
            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void return_value(int value) {
                result = value;
            }

            void unhandled_exception()
            {
                exception = std::current_exception();
                result = std::nullopt ;
            }

            [[nodiscard]]
            std::optional<int> getResult() const noexcept {
                return result;
            }

        private:
            std::optional<int> result { std::nullopt };
            std::exception_ptr exception;
        };

        explicit ComputeResult(const std::coroutine_handle<Promise> hCoro) : handle { hCoro } {
        }

        ~ComputeResult()
        {
            if (handle) {
                handle.destroy();
            }
        }

        ComputeResult(const ComputeResult&) = delete;
        ComputeResult& operator=(const ComputeResult&) = delete;

        ComputeResult(ComputeResult&& other) noexcept :
            handle { std::exchange(other.handle, nullptr) } {
        }

        ComputeResult& operator=(ComputeResult&& other) noexcept
        {
            if (handle) {
                handle.destroy();
            }
            handle = std::exchange(other.handle, nullptr);
            return *this;
        }

        void run() const
        {
            while (!handle.done()) {
                handle.resume();
                // LOG << "Resuming...." << std::boolalpha << handle.done() << std::endl;
            }
        }

        [[nodiscard]]
        std::optional<int> getResult() const noexcept
        {
            return handle.promise().getResult();
        }

    private:
        std::coroutine_handle<promise_type> handle;
    };

    ComputeResult calcSum(const int n)
    {
        int sum = 0;
        for (int i = 0; i <= n; ++i) {
            sum += i;
            // LOG << "sum = " << sum << std::endl;
            co_await std::suspend_always{};
        }
        co_return sum;
    }

    void demo()
    {
        const ComputeResult computation = calcSum(5);
        computation.run();

        if (const std::optional<int> result  = computation.getResult(); result.has_value()) {
            LOG << "Result() = " << result.has_value() << std::endl;
        }
    }
}

void StdCoroutines::Simple::Returning_Coroutine_ComputeResult::TestAll()
{
    demo();
}
