/**============================================================================
Name        : Cooperative_Multitasking.cpp
Created on  : 10.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Cooperative_Multitasking.cpp
============================================================================**/

#include "Cooperative_Multitasking.hpp"

#include <vector>
#include <iostream>
#include <string>
#include <print>
#include <utility>
#include <coroutine>

namespace
{
    // INFO: https://www.vinniefalco.com/p/how-to-understand-c20-coroutines

    struct Task
    {
        struct promise_type
        {
            Task get_return_object() {
                return Task { *this };
            }

            std::suspend_always initial_suspend() {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void return_void() {
            }

            void unhandled_exception() {
                std::terminate();
            }
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

        Task(Task&& other) noexcept : handle { std::exchange(other.handle, nullptr) } {
        }

        [[nodiscard]]
        bool done() const {
            return handle.done();
        }

        void resume() {
            handle.resume();
        }
    };

    struct Scheduler
    {
        std::vector<Task> tasks;

        void add(Task task) {
            tasks.push_back(std::move(task));
        }

        void run()
        {
            while (!tasks.empty())
            {
                for (int64_t i = 0; i < tasks.size(); )
                {
                    tasks[i].resume();
                    if (tasks[i].done()) {
                        // Will not compile - no Copy Constructor
                        // tasks.erase(tasks.begin() + i);
                    } else {
                        ++i;
                    }
                }
            }
        }
    };

    Task worker(const std::string& name, const int iterations)
    {
        for (int i = 0; i < iterations; ++i) {
            std::cout << name << " iteration " << i << std::endl;
            co_await std::suspend_always{};
        }
    }

    void demo()
    {
        Scheduler scheduler;
        scheduler.add(worker("Alice", 3));
        scheduler.add(worker("Bob", 2));
        scheduler.run();
    }
}

void use_cases::cooperative_multitasking::TestAll()
{
}
