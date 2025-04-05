/**============================================================================
Name        : Generic_TaskBased_Coroutine.cpp
Created on  : 02.03.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Generic_TaskBased_Coroutine
============================================================================**/

#include "Experiments.h"
#include "Utilities.h"

#include <coroutine>
#include <print>
#include <thread>
#include <functional>
#include <deque>
#include <utility>

namespace
{
    auto tid() { return std::this_thread::get_id();}
    auto time() { return Utilities::getCurrentTime();}
}

namespace
{
    template <typename T>
    struct Task
    {
        struct promise_type;
        using promise_handle_t = std::coroutine_handle<promise_type>;

        explicit Task(promise_handle_t h) : handle(h) {
        }

        Task(Task &&task) noexcept : handle(std::exchange(task.handle, {})) {
        }

        ~Task()
        {
            if (handle) {
                handle.destroy();
            }
        }

        template <typename R>
        class TaskAwaiter
        {
            Task<R> task;

        public:

            explicit TaskAwaiter(Task<R>&& task) noexcept : task { std::move(task) } {
            }

            TaskAwaiter(TaskAwaiter &) = delete;
            TaskAwaiter &operator=(TaskAwaiter &) = delete;

            bool await_ready() noexcept {
                return false;
            }

            void await_suspend(const std::coroutine_handle<Task::promise_type>& hCoro) noexcept
            {
                task.finally([hCoro]() {
                    hCoro.resume();
                });
            }

            R await_resume() noexcept {
                return task.get_result();
            }
        };

        struct promise_type
        {
            Task get_return_object() {
                return Task(promise_handle_t::from_promise(*this));
            }

            std::suspend_never initial_suspend() {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            template <typename U>
            TaskAwaiter<U> await_transform(Task<U> &&task) {
                return TaskAwaiter<U>(std::move(task));
            }

            void unhandled_exception() {
            }

            void return_value(T t) {
                data = t;
                notify_callbacks();
            }

            void on_completed(std::function<void(T)> && callback)
            {
                std::println("[{}] [{}] on_completed.{}", tid(), time(), __LINE__);
                if (data.has_value()) {
                    callback(data.value());
                } else {
                    callbacks.push_back(callback);
                }
            }

            T get() {
                return data.value();
            }

        private:

            void notify_callbacks()
            {
                std::println("[{}] [{}] notify_callbacks(). tasks count: {}", tid(), time(), callbacks.size());
                for (auto &callback : callbacks) {
                    callback(data.value());
                }
                callbacks.clear();
            }

            std::optional<T> data;
            std::deque<std::function<void(T)>> callbacks;
        };

        [[nodiscard]]
        T get_result() const {
            return handle.promise().get();
        }

        Task& then(std::function<void(T)> && callback)
        {
            handle.promise().on_completed([callback](auto data) {
                callback(data);
            });
            return *this;
        }

        void finally(std::function<void()> &&callback)
        {
            handle.promise().on_completed([callback](auto result) {
                callback();
            });
        }

    private:
        promise_handle_t handle;
    };

    Task<int> task1() {
        std::println("[{}] [{}] task1() run", tid(), time());
        co_return 1;
    }

    Task<int> task2() {
        std::println("[{}] [{}] task2() run", tid(), time());
        co_return 2;
    }

    Task<int> callTasks()
    {
        std::println("[{}] [{}] callTasks - task1()", tid(), time());
        int data1 = co_await task1();

        std::println("[{}] [{}] callTasks - task2().task1 data = {}", tid(), time(), data1);
        int data2 = co_await task2();

        std::println("[{}] [{}] callTasks - task2 data = {}", tid(), time(), data2);
        co_return data1 + data2;
    }
}


// https://www.alibabacloud.com/blog/a-comprehensive-discussion-on-c%2B%2B20-coroutines_600889

void StdCoroutines::Experiments::Generic_TaskBased_Coroutine::TestAll()
{
    Task<int> task = callTasks();
    task.then([](int data) { std::println("[{}] [{}] Final callback 1 = {}", tid(), time(), data);})
        .then([](int data) { std::println("[{}] [{}] Final callback 2 = {}", tid(), time(), data);});
}
