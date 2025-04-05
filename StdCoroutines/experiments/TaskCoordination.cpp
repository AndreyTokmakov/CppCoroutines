/**============================================================================
Name        : UseCases_TaskCoordination.cpp
Created on  : 01.03.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine Waitable
============================================================================**/

#include "Experiments.h"

#include <chrono>
#include <thread>
#include <source_location>

namespace
{
    auto tid() { return std::this_thread::get_id();}
    auto time() { return Utilities::getCurrentTime();}

    void log(const std::string_view message,
             const std::source_location location = std::source_location::current())
    {
        std::println("File: {} [{}:{}] {} : {}",
                     location.file_name(), location.line(), location.column(), location.function_name(),
                     message);
    }
}
namespace
{
    struct Task
    {
        struct promise_type;

        struct TaskAwaiter
        {
            std::chrono::milliseconds timeout;

            explicit TaskAwaiter(const std::chrono::milliseconds& delay) : timeout { delay } {
                std::println("[{}] [{}] TaskAwaiter::TaskAwaiter(timeout: {})", tid(), time(), timeout.count());
            }

            bool await_ready()
            {
                std::println("[{}] [{}] TaskAwaiter::await_ready() -> false", tid(), time());
                return false;
            }

            void await_suspend(const std::coroutine_handle<promise_type>& coroHandle)
            {
                std::println("[{}] [{}] TaskAwaiter::await_suspend(timeout: {})", tid(), time(), timeout.count());
                std::thread([coroHandle, this]() {
                    std::this_thread::sleep_for(timeout);
                    std::println("[{}] [{}] TaskAwaiter::await_suspend(timeout: {}) done ", tid(), time(), timeout.count());
                    coroHandle.resume();
                }).detach();
            }

            void await_resume() {
                // std::println("[{}] [{}] TaskAwaiter::await_resume()", tid(), time());
            }
        };

        struct promise_type
        {
            Task get_return_object() {
                // std::println("[{}] [{}] get_return_object() --> Task())", tid(), time());
                return Task { std::coroutine_handle<promise_type>::from_promise(*this)  };
            }

            std::suspend_never initial_suspend()
            {
                // std::println("[{}] [{}] Task::initial_suspend()", tid(), time());
                return {};
            }

            std::suspend_never final_suspend() noexcept
            {
                std::println("[{}] [{}] Task::final_suspend()", tid(), time());
                return {};
            }

            void return_void() {
                // std::println("[{}] [{}] Task::return_void()", tid(), time());
            }

            void unhandled_exception() {
                // std::println("[{}] [{}] Task::unhandled_exception()", tid(), time());
                std::terminate();
            }

            TaskAwaiter await_transform(const std::chrono::milliseconds& delay) noexcept {
                return TaskAwaiter { delay };
            }
        };

        std::coroutine_handle<promise_type> handle;
    };


    Task moveEntity(int id, int distance)
    {
        std::println("[{}] [{}] Entity {} moving {} units", tid(), time(), id, distance);
        co_await std::chrono::milliseconds(500U * distance);
    }

    Task updateEntity(int id)
    {
        std::println("[{}] [{}] Entity {} updating", tid(), time(), id);
        co_await std::chrono::milliseconds(500U);
    }


    Task runSimulation()
    {
        moveEntity(1, 5);
        updateEntity(1);
        moveEntity(2, 3);
        updateEntity(2);
        moveEntity(1, 2);
    }
}


void StdCoroutines::Experiments::TaskCoordination::TestAll()
{
    runSimulation();
}
