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

namespace StdCoroutines::Generators::Simple_Generator
{
    // INFO: https://www.vinniefalco.com/p/how-to-understand-c20-coroutines

    struct Generator
    {
        struct promise_type
        {

            Generator get_return_object() noexcept {
                return Generator { *this };
            }
#if 0
            Generator get_return_object() noexcept {
                return Generator {
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }
#endif

            std::suspend_always initial_suspend() {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            std::suspend_always yield_value(const int v) {
                value = v;
                return {};
            }

            void return_void() {
            }

            void unhandled_exception()
            {
                std::terminate();
            }

            [[nodiscard]]
            int getValue() const noexcept
            {
                return value;
            }

        private:

            int value { 0 };
        };

        std::coroutine_handle<promise_type> handle;

#if 0
            explicit Generator(std::coroutine_handle<promise_type> h) : handle(h) {
        }
#endif

        explicit Generator(promise_type& promise) :
            handle { std::coroutine_handle<promise_type>::from_promise(promise) } {
        }

        ~Generator()
        {
            if (handle) {
                handle.destroy();
            }
        }

        // Disable copying
        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;

        // Enable moving
        Generator(Generator&& other) noexcept  : handle { std::exchange(other.handle, nullptr) } {
        }

        Generator& operator=(Generator&& other) noexcept
        {
            if (this != &other) {
                if (handle) {
                    handle.destroy();
                }
                handle = other.handle;
                other.handle = nullptr;
            }
            return *this;
        }

        bool next() {
            if (!handle || handle.done())
                return false;
            handle.resume();
            return !handle.done();
        }

        [[nodiscard]]
        int value() const {
            return handle.promise().getValue();
        }
    };

    Generator count_to(int n)
    {
        for (int i = 1; i <= n; ++i) {
            co_yield i;
        }
    }

    void demo()
    {
        Generator gen = count_to(5);
        while (gen.next()) {
            std::cout << gen.value() << std::endl;
        }

        // 1
        // 2
        // 3
        // 4
        // 5
    }
}

namespace StdCoroutines::Generators::Simple_Generator_2
{
    template <typename T>
    struct Generator
    {
        using value_type = T;

        struct promise_type
        {
            Generator get_return_object() {
                return Generator { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            // Suspend immediately -- don't run the body until asked
            [[nodiscard]]
            std::suspend_always initial_suspend() const noexcept {
                return {};
            }

            // Suspend at the end -- let the Generator destructor clean up
            [[nodiscard]]
            std::suspend_always final_suspend() const noexcept {
                return {};
            }

            // When the coroutine does: co_yield value;
            std::suspend_always yield_value(T value)
            {
                current_value = std::move(value);
                return {};
            }

            // Our generator doesn't return a final value, just yields
            void return_void() {
            }

            // If something throws inside the coroutine body
            void unhandled_exception() {
                std::terminate(); // Nuclear option. You can rethrow instead.
            }

            [[nodiscard]]
            value_type getStoredValue() const noexcept {
                return current_value;
            }

        private:
            value_type current_value;
        };

        explicit Generator(std::coroutine_handle<promise_type> handle): handle(handle) {
        }

        ~Generator()
        {
            if (handle) {
                handle.destroy();
            }
        }

        // No copying! The handle is a unique resource.
        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;

        // Move is fine though
        Generator(Generator&& other) noexcept: handle { std::exchange(other.handle, nullptr) }  {
        }

        Generator& operator=(Generator&& other) noexcept
        {
            if (this != &other)
            {
                if (handle) {
                    handle.destroy();
                }
                handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }

        // Advance to the next value. Returns false if done.
        bool next()
        {
            if (!handle || handle.done())
                return false;
            handle.resume();
            return !handle.done();
        }

        // Get the current yielded value
        [[nodiscard]]
        value_type value() const {
            return handle.promise().getStoredValue();
        }

    private:
        std::coroutine_handle<promise_type> handle;
    };

    Generator<int> count_up_to(const int max)
    {
        for (int i = 1; i <= max; ++i) {
            co_yield i;
        }
    }

    void demo()
    {
        Generator<int>  gen = count_up_to(5);
        while (gen.next()) {
            std::cout << gen.value() << " ";
        }
        // 1 2 3 4 5
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



void StdCoroutines::Generators::TestAll()
{
    // SimpleExample::printLetters();

    // Simple_Generator::demo();
    Simple_Generator_2::demo();

    // Generic_Generator_ExcHandler::demo();
    // Generic_Generator_ExcHandler::handleExceptionDemo();

    // Fibonacci_Sequence_Generator::Test();
    // Fibonacci_Sequence_Generator_Ex::Test();
    // Fibonacci_Sequence_Generator_2::test();
    // Fibonacci_Sequence_Generator_3::test();
}

