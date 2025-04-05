/**============================================================================
Name        : Experiments.cpp
Created on  : 27.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Coroutines - Experiments
============================================================================**/

#include "Experiments.h"

#include <coroutine>
#include <print>
#include <iostream>
#include <thread>
#include <functional>
#include <deque>
#include <utility>

namespace
{
    struct HelloWorldCoro
    {
        struct promise_type
        {
            int m_val;

            HelloWorldCoro get_return_object() {
                return HelloWorldCoro { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            std::suspend_always yield_value(int val) {
                m_val = val;
                return {};
            }

            void unhandled_exception()
            {
                std::terminate();
            }
        };

        explicit HelloWorldCoro(const std::coroutine_handle<promise_type> handle) : coroHandle { handle }  {
        }

        ~HelloWorldCoro() {
            coroHandle.destroy();
        }

        std::coroutine_handle<promise_type> coroHandle;
    };

    struct dummy { // Awaitable
        std::suspend_always operator co_await(){ return {}; }
    };

    HelloWorldCoro print_hello_world() {
        std::cout << "Hello ";
        co_await dummy{};
        std::cout << "World!" << std::endl;
    }
}


void StdCoroutines::Experiments::TestAll()
{

}


