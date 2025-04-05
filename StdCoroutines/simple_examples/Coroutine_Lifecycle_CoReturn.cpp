/**============================================================================
Name        : Coroutine_Lifecycle_CoReturn.cpp
Created on  : 27.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple_Coroutine.cpp
============================================================================**/

#include "SimpleCoroutines.h"
#include "Utilities.h"

#include <coroutine>
#include <print>

namespace
{
    using Utilities::getCurrentTime;
}

namespace
{
    struct ReturnType
    {
        struct promise_type
        {
            ReturnType get_return_object() noexcept
            {
                std::println("[{}] get_return_object --> ReturnType()", getCurrentTime());
                return ReturnType { *this };
            }

            void return_void() noexcept
            {
                std::println("[{}] return_void()", getCurrentTime());
            }

            std::suspend_always initial_suspend() noexcept
            {
                std::println("[{}] initial_suspend() --> std::suspend_always() awaitable object" , getCurrentTime());
                return {};
            }

            std::suspend_always final_suspend() noexcept
            {
                std::println("[{}] final_suspend() --> std::suspend_always() awaitable object" , getCurrentTime());
                return {};
            }

            void unhandled_exception() noexcept
            {
                std::println("[{}] unhandled_exception()" , getCurrentTime());
                std::terminate();
            }
        };

        explicit ReturnType(promise_type&) {
            std::println("[{}] ReturnType()" , getCurrentTime());
        }

        ~ReturnType() noexcept {
            std::println("[{}] ~ReturnType()" , getCurrentTime());
        }
    };

    ReturnType createCoroutine()
    {
        std::println("[{}] createCoroutine() --> ReturnType()" , getCurrentTime());
        co_return;
    }

}

void StdCoroutines::Simple::Coroutine_Lifecycle_CoReturn::TestAll()
{
    ReturnType rt = createCoroutine();
};