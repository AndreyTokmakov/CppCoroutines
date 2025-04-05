/**============================================================================
Name        : Generators.cpp
Created on  : 27.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Coroutines - Generators
============================================================================**/

#include "Generators.h"

#include <iostream>
#include <string_view>
#include <future>
#include <semaphore>
#include <chrono>
#include <coroutine>
#include <generator>
#include <utility>


namespace StdCoroutines::Generators::SimpleExample
{
    std::generator<char> letters(char first)
    {
        while (true)
            co_yield first++;
    }

    void printLetters()
    {
        for (const char ch : letters('a') | std::views::take(26)) {
            std::cout << ch << ' ';
        }
        std::cout << '\n';
    }
}

namespace StdCoroutines::Generators::Fibonacci_Sequence_Generator
{
    std::generator<int> fibonacci_generator()
    {
        int a { 0 }, b{  1 };
        while (true) {
            co_yield a;
            const int c = a + b;
            a = std::exchange(b, c);
        }
    }

    std::generator<int> fibonacci_generator(int limit)
    {
        int a { 0 }, b{  1 };
        while  (limit--) {
            co_yield a;
            const int c = a + b;
            a = std::exchange(b, c);
        }
    }

    void Test()
    {
        for (const int val : fibonacci_generator() | std::views::take(10))
            std::cout << val << ' ';

        std::cout << std::endl;

        for (const int val : fibonacci_generator(10)) {
            std::cout << val << ' ';
        }
    }
}

namespace StdCoroutines::Fibonacci_Sequence_Generator_Ex
{
    using namespace std::string_literals;

    template <typename Out>
    struct SequenceGenerator
    {
        struct promise_type
        {
            Out output_data { };

            SequenceGenerator get_return_object() noexcept {
                return SequenceGenerator { *this };
            }

            void return_void() noexcept {
            }

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void unhandled_exception() noexcept {
            }

            std::suspend_always yield_value(int64_t num) noexcept {
                output_data = num;
                return {};
            }
        };

        std::coroutine_handle<promise_type> handle{};

        explicit SequenceGenerator(promise_type& promise) :
            handle { std::coroutine_handle<promise_type>::from_promise(promise) } {
        }

        ~SequenceGenerator() noexcept
        {
            if (handle) {
                handle.destroy();
            }
        }

        void next() {
            if (!handle.done()) {
                handle.resume();
            }
        }

        int64_t value() {
            return handle.promise().output_data;
        }
    };

    SequenceGenerator<int64_t> fibonacci()
    {
        int64_t a{ 0 };
        int64_t b{ 1 };
        int64_t c{ 0 };

        while (true) {
            co_yield a;
            c = a + b;
            a = b;
            b = c;
        }
    }

    void Test()
    {
        SequenceGenerator<int64_t> fib = fibonacci();

        std::cout << "Generate ten Fibonacci numbers\n"s;

        for (int i = 0; i < 10; ++i) {
            fib.next();
            std::cout << fib.value() << " ";
        }
        std::cout << std::endl;

        std::cout << "Generate ten more\n"s;

        for (int i = 0; i < 10; ++i) {
            fib.next();
            std::cout << fib.value() << " ";
        }
        std::cout << std::endl;

        std::cout << "Let's do five more\n"s;

        for (int i = 0; i < 5; ++i) {
            fib.next();
            std::cout << fib.value() << " ";
        }
        std::cout << std::endl;

    }
}

namespace StdCoroutines::Generators::Fibonacci_Sequence_Generator_2
{

    template <typename T>
    struct Generator
    {
        struct promise_type
        {
            Generator get_return_object() {
                return Generator { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void unhandled_exception() {
            }

            void return_value(T t) noexcept {
                value = t;
            }

            std::suspend_always yield_value(T t) {
                value = t;
                return {};
            }

            T value {};
        };

        [[nodiscard]]
        bool has_next() const {
            return !handle.done();
        }

        [[nodiscard]]
        size_t next() {
            handle.resume();
            return handle.promise().value;
        }

        std::coroutine_handle<promise_type> handle;
    };

    Generator<size_t> fib(size_t max_count)
    {
        co_yield 1;
        size_t a = 0, b = 1, count = 0;
        while (++count < max_count - 1) {
            co_yield a + b;
            b = a + b;
            a = b - a;
        }
        co_return a + b;
    }

    void test()
    {
        size_t max_count = 10;
        auto generator = fib(max_count);
        size_t i = 0;
        while (generator.has_next()) {
            std::cout << "No." << ++i << ": " << generator.next() << std::endl;
        }
        /**
        No.1: 1
        No.2: 1
        No.3: 2
        No.4: 3
        No.5: 5
        No.6: 8
        No.7: 13
        No.8: 21
        No.9: 34
        No.10: 55
        **/
    }
}


namespace StdCoroutines::Generators::Fibonacci_Sequence_Generator_3
{

    template <typename T>
    struct Generator
    {
        using value_type = T;

        struct promise_type
        {
            Generator get_return_object() {
                return Generator { *this };
            }

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void unhandled_exception() {
            }

            void return_value(value_type t) noexcept {
                value = t;
            }

            std::suspend_always yield_value(value_type&& val) {
                value = std::move(val);
                return {};
            }

            std::optional<value_type> value {};
        };

        [[nodiscard]]
        bool has_next() const {
            return !handle.done();
        }

        [[nodiscard]]
        std::optional<value_type> next() {
            handle.resume();
            return handle.promise().value;
        }

        explicit Generator(promise_type& promise) :
            handle { std::coroutine_handle<promise_type>::from_promise(promise) } {
        }

        ~Generator() noexcept {
            if (handle) {
                handle.destroy();
            }
        }

        std::coroutine_handle<promise_type> handle;
    };

    Generator<int> fib(const int max_count)
    {
        co_yield 1;
        int a = 0, b = 1;
        for (int count = 0; count < max_count - 1; ++count) {
            co_yield a + b;
            b = a + b;
            a = b - a;
        }
        co_return a + b;
    }

    void test()
    {
        constexpr int max_count = 10;
        Generator<int> generator = fib(max_count);
        for (int iter = 0; generator.has_next(); ++iter) {
            std::cout << "iter: " << ++iter << ", value: " << generator.next().value() << std::endl;
        }
    }
}



void StdCoroutines::Generators::TestAll()
{
    // SimpleExample::printLetters();
    // Fibonacci_Sequence_Generator::Test();
    // Fibonacci_Sequence_Generator_Ex::Test();
    // Fibonacci_Sequence_Generator_2::test();
    Fibonacci_Sequence_Generator_3::test();
}

