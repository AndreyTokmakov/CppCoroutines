/**============================================================================
Name        : Exception_Handling.hpp.cpp
Created on  :
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "Exception_Handling.hpp"

#include <coroutine>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace StdCoroutines::Exception_Handling::simple_example
{
    struct Task
    {
        struct promise_type
        {
            std::exception_ptr exception;

            Task get_return_object() {
                return Task {*this };
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
                exception = std::current_exception();
            }
        };

        explicit Task(promise_type& promise) :
            handle { std::coroutine_handle<promise_type>::from_promise(promise) } {
        }

        ~Task()
        {
            if (handle) {
                handle.destroy();
            }
        }

        void run() const {
            handle.resume();
        }

        void check_exception() const
        {
            if (handle.promise().exception) {
                std::rethrow_exception(handle.promise().exception);
            }
        }

        std::coroutine_handle<promise_type> handle;

    };

    Task risky_operation()
    {
        std::cout << "Starting risky operation" << std::endl;
        throw std::runtime_error("Something went wrong");
        co_return;  // Never reached
    }

    void demo()
    {
        const Task task = risky_operation();
        try
        {
            task.run();
            task.check_exception();
            std::cout << "Operation completed successfully" << std::endl;
        }
        catch (const std::exception& exc) {
            std::cout << "Operation failed: " << exc.what() << std::endl;
        }

        // Starting risky operation
        // Operation failed: Something went wrong
    }
}


void StdCoroutines::Exception_Handling::TestAll()
{
    simple_example::demo();
}


