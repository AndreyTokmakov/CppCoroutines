/**============================================================================
Name        : CustomAllocation.cpp
Created on  : 14.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : CustomAllocation.cpp
============================================================================**/

#include "CustomAllocation.hpp"

#include <iostream>
#include <memory>

namespace
{
    struct FramePool
    {
        static constexpr size_t blockSize { 512 };
        static constexpr size_t poolSize { 1024 };

        alignas(std::max_align_t)
        char buffer[poolSize][blockSize]{};

        void* free_list[poolSize]{};
        size_t top = 0;

        FramePool()
        {
            for (auto & i : buffer)
                free_list[top++] = i;
        }

        void* allocate(const size_t size)
        {
            std::cout << "Allocating coroutine frame: " << size << "\n";
            if (size > blockSize || top == 0)
                return std::malloc(size);
            return free_list[--top];
        }

        void deallocate(void* ptr, const size_t size)
        {
            std::cout << "Freeing coroutine frame: " << size << "\n";
            if (size > blockSize) {
                std::free(ptr);
                return;
            }
            free_list[top++] = ptr;
        }
    };

    FramePool pool;
}

namespace
{
    template<typename T>
    struct [[nodiscard]]  Task
    {
        struct promise_type
        {
            T storedValue;

            [[nodiscard]]
            Task get_return_object() noexcept {
                return Task { *this };
            }

            std::suspend_always initial_suspend() noexcept {
                std::println("initial_suspend() ==> std::suspend_always");
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                std::println("final_suspend() ==> std::suspend_always");
                return {};
            }

            void return_value(T value) {
                std::println("return_value({})", value);
                storedValue = value;
            }

            void unhandled_exception() {
                std::terminate();
            }
#if 1
            static void* operator new(const std::size_t size) {
                // return std::malloc(size);
                return pool.allocate(size);
            }

            static void operator delete(void* ptr, const std::size_t size) noexcept {
                // std::free(ptr);
                pool.deallocate(ptr, size);
            }
#endif
        };

        std::coroutine_handle<promise_type> handle;

        explicit Task(promise_type& promise) :
           handle { std::coroutine_handle<promise_type>::from_promise(promise) } {
        }

        ~Task()
        {
            if (handle) {
                handle.destroy();
            }
        }

        [[nodiscard]]
        T getValue() {
            return handle.promise().storedValue;
        }

        void resume() {
            handle.resume();
        }

        [[nodiscard]]
        bool done() const {
            return handle.done();
        }

        void run()
        {
            while (!done()) {
                resume();
            }
        }
    };

    Task<int> runTaskCoAwait()
    {
        std::println("Task body: part 1");
        co_await std::suspend_always{};
        std::println("Task body: part 2");
    }

    template <typename T>
    Task<T> runTaskCoReturn(T v)
    {
        //std::println("Task body: part 1");
        co_return v;
    }
}

void demo_CoAwait()
{
    std::println("Before calling coroutine");
    Task task = runTaskCoAwait();
    std::println("After calling coroutine, before first resume");
    task.resume();
    std::println("After first resume, before second resume");
    task.resume();
    std::println("After second resume");

    /**
    Before calling coroutine
    Allocating coroutine frame: 32
    initial_suspend() ==> std::suspend_always
    After calling coroutine, before first resume
    Task body: part 1
    After first resume, before second resume
    Task body: part 2
    final_suspend() ==> std::suspend_always
    After second resume
    Freeing coroutine frame: 32
    **/
}

void demo_CoReturn()
{
    std::println("Before calling coroutine");
    Task<int> task = runTaskCoReturn<int>(123);

    task.run();

    std::println("Result = {}", task.getValue());
    std::println("After second resume");

    /**
    *Before calling coroutine
    Allocating coroutine frame: 32
    initial_suspend() ==> std::suspend_always
    return_value(123)
    final_suspend() ==> std::suspend_always
    Result = 123
    After second resume
    Freeing coroutine frame: 32
    **/
}

void custom_allocation::FrameAllocation::TestAll()
{
    demo_CoAwait();
    // demo_CoReturn();
}

