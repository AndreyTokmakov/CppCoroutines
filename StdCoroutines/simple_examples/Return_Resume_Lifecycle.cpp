/**============================================================================
Name        : Return_Resume_Lifecycle.cpp
Created on  :
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Return Resume Lifecycle
============================================================================**/

#include "SimpleCoroutines.hpp"
#include <iostream>
#include <chrono>
#include <thread>



namespace
{
    struct Task
    {
        struct promise_type
        {
            [[nodiscard]]
            Task get_return_object() noexcept {
                std::cout << "Creating return object" << std::endl;
                return Task { *this };
            }

            std::suspend_always initial_suspend()
            {
                std::cout << "Initial suspend ==> std::suspend_always{}" << std::endl;
                return {};
            }

            std::suspend_always final_suspend() noexcept
            {
                std::cout << "Final suspend ==> std::suspend_always{}" << std::endl;
                return {};
            }

            void return_void() {}
            void unhandled_exception() {}
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

        Task(Task&& other) noexcept : handle(other.handle) {
            other.handle = nullptr;
        }

        void resume()
        {
            handle.resume();
        }

        bool done() const
        {
            return handle.done();
        }
    };

    Task runTask()
    {
        std::cout << "Task body: part 1" << std::endl;
        co_await std::suspend_always{};
        std::cout << "Task body: part 2" << std::endl;
    }
}

void StdCoroutines::Simple::Return_Resume_Lifecycle::TestAll()
{
    std::cout << "Before calling coroutine" << std::endl;

    Task task = runTask();

    std::cout << "After calling coroutine, before first resume" << std::endl;
    task.resume();

    std::cout << "After first resume, before second resume" << std::endl;
    task.resume();

    std::cout << "After second resume" << std::endl;

    /**
    Before calling coroutine
    Creating return object
    Initial suspend ==> std::suspend_always{}
    After calling coroutine, before first resume
    Task body: part 1
    After first resume, before second resume
    Task body: part 2
    Final suspend ==> std::suspend_always{}
    After second resume
    **/
}

/**
* Follow the execution flow:
* 1. Before runTask() is called, nothing has happened.
* 2. Calling runTask() creates the coroutine frame, constructs the promise, and calls get_return_object().
* 3. The return object (Task) is created with a handle to the coroutine.
* 4. initial_suspend() runs and returns suspend_always, so the coroutine suspends immediately.
* 5. Control returns to main, which now holds the Task object.
* 6. The first  resume() runs “Task body: part 1”, then hits co_await suspend_always{} and suspends.
* 7. The second resume() runs “Task body: part 2”, then falls off the end, triggering final_suspend().
* 8. Since final_suspend() returns suspend_always, the coroutine suspends one final time.
**/
