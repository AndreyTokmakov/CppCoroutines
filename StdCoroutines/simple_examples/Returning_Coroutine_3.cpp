/**============================================================================
Name        : Returning_Coroutine_3.cpp
Created on  : 21.04.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine returning value
============================================================================**/

#include "SimpleCoroutines.hpp"

#include <iostream>
#include <syncstream>
#include <utility>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '
#define ERR std::osyncstream { std::cerr } << Utilities::getCurrentTime() << ' '

namespace
{
    template <typename T>
    struct Task
    {
        struct Promise;
        using promise_type = Promise;

        struct Promise
        {
            std::optional<T> result;
            std::exception_ptr exception;

            Task get_return_object()  {
                return Task{
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

            // co_return value; stores the result
            void return_value(T value) {
                result = std::move(value);
            }

            void unhandled_exception() {
                exception = std::current_exception();
            }
        };

        explicit Task(std::coroutine_handle<Promise> h) : handle(h) {
        }

        ~Task()
        {
            if (handle) {
                handle.destroy();
            }
        }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        Task(Task&& other) noexcept : handle { std::exchange(other.handle, nullptr) } {
        }

        Task& operator=(Task&& other) noexcept
        {
            if (handle) {
                handle.destroy();
            }
            handle = std::exchange(other.handle, nullptr);
            return *this;
        }

        // Run the coroutine to completion and get the result
        T get()
        {
            handle.resume();
            auto& promise = handle.promise();
            if (promise.exception) {
                std::rethrow_exception(promise.exception);
            }
            return std::move(*promise.result);
        }

    private:
        std::coroutine_handle<promise_type> handle;
    };

    Task<int> compute_answer() {
        int result = 42;
        co_return result;
    }

    Task<std::string> greet(const std::string name) {
        co_return "Hello, " + name + "!";
    }

    void demo()
    {
        auto task1 = compute_answer();
        LOG << task1.get() << "\n";

        auto task2 = greet("World");
        LOG<< task2.get() << "\n";

        // 2026-04-21 18:37:18.683421 42
        // 2026-04-21 18:37:18.683495 Hello, World!
    }
}


void StdCoroutines::Simple::Returning_Coroutine_3::TestAll()
{
    demo();
}
