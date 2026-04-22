/**============================================================================
Name        : Generic_Generator.cpp
Created on  :
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "Generators.hpp"


#include <iostream>
#include <syncstream>
#include <utility>
#include <thread>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime()  \
<< " [" << std::this_thread::get_id() << "] "

namespace StdCoroutines::Generators::Generic_Generator_ExcHandler
{
    // INFO: https://www.vinniefalco.com/p/how-to-understand-c20-coroutines

    template<typename T>
    struct Generator
    {
        struct promise_type
        {
            T value {};

            std::exception_ptr exception;

            Generator get_return_object() {
                return Generator { *this };
            }

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            std::suspend_always yield_value(T v) {
                value = std::move(v);
                return {};
            }

            void return_void() noexcept {
            }

            void unhandled_exception() {
                std::cerr << "promise_type::unhandled_exception()\n";
                exception = std::current_exception();
            }

            template<typename U>
            std::suspend_never await_transform(U&&) = delete;
        };

        using Handle = std::coroutine_handle<promise_type>;

    private:
        Handle handle_;

    public:

        explicit Generator(promise_type& promise) :
            handle_ { std::coroutine_handle<promise_type>::from_promise(promise) } {
        }

        ~Generator() {
            if (handle_) {
                handle_.destroy();
            }
        }

        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;

        Generator(Generator&& other) noexcept
            : handle_ { std::exchange(other.handle_, nullptr) } {}

        Generator& operator=(Generator&& other) noexcept
        {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, nullptr);
            return *this;
        }

        class iterator
        {
            Handle handle { nullptr };

        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            iterator() = default;
            explicit iterator(Handle h) : handle(h) {}

            iterator& operator++()
            {
                handle.resume();
                if (handle.done())
                {
                    auto& promise = handle.promise();
                    handle = nullptr;
                    if (promise.exception) {
                        std::rethrow_exception(promise.exception);
                    }
                }
                return *this;
            }

            iterator operator++(int)
            {
                iterator temp = *this;
                ++(*this);
                return temp;
            }

            T& operator*() const {
                return handle.promise().value;
            }

            T* operator->() const {
                return &handle.promise().value;
            }

            bool operator==(const iterator& other) const {
                return handle == other.handle;
            }

            bool operator!=(const iterator& other) const {
                return !(*this == other);
            }
        };

        iterator begin()
        {
            if (handle_)
            {
                handle_.resume();
                if (handle_.done())
                {
                    if (auto& promise = handle_.promise(); promise.exception) {
                        std::rethrow_exception(promise.exception);
                    }
                    return iterator{};
                }
            }
            return iterator { handle_ };
        }

        iterator end()
        {
            return iterator{};
        }
    };

    Generator<int> range(const int start, const int end)
    {
        for (int i = start; i < end; ++i) {
            co_yield i;
        }
    }

    Generator<int> squares(const int n)
    {
        for (int i = 0; i < n; ++i) {
            co_yield i * i;
        }
    }

    Generator<int> may_throw(bool should_throw)
    {
        co_yield 1;
        co_yield 2;
        if (should_throw) {
            throw std::runtime_error("Generator error");
        }
        co_yield 3;
    }

    void demo()
    {
        std::cout << "Range 1 to 5: ";
        for (const int x : range(1, 6)) {
            std::cout << x << " ";
        }
        std::cout << std::endl;

        std::cout << "First 5 squares: ";
        for (const int x : squares(5)) {
            std::cout << x << " ";
        }
        std::cout << std::endl;

        // Range 1 to 5: 1 2 3 4 5
        // First 5 squares: 0 1 4 9 16
    }

    void handleExceptionDemo()
    {
        try {
            for (int x : may_throw(true)) {
                std::cout << x << std::endl;
            }
        }
        catch (const std::exception& exc) {
            std::cout << "Caught: " << exc.what() << std::endl;
        }

        // promise_type::unhandled_exception()
        // 1
        // 2
        // Caught: Generator error
    }
}



void StdCoroutines::Generators::Generic_Generator::TestAll()
{
    Generic_Generator_ExcHandler::demo();
    // Generic_Generator_ExcHandler::handleExceptionDemo();
}