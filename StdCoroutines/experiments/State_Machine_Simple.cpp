/**============================================================================
Name        : PinBall_Game.cpp
Created on  : 27.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "Experiments.h"

#include <chrono>
#include <thread>
#include <iostream>

namespace
{
    auto tid() { return std::this_thread::get_id();}
    auto time() { return Utilities::getCurrentTime();}
}

namespace State_Stored_in_Awaitable
{
    struct ConnectionStateCoroutine
    {
        template<class PromiseType>
        struct Awaiter
        {
            std::string state;

            explicit Awaiter(std::string s) : state { std::move(s) } {
                std::println("[{}] [{}] \t Awaiter({})", tid(), time(), state);
            }

            [[nodiscard]]
            bool await_ready() const noexcept
            {
                constexpr bool await_success = false;
                std::println("[{}] [{}] \t await_ready() --> {}", tid(), time(), await_success);
                return await_success;
            }

            void await_suspend(const std::coroutine_handle<PromiseType>& hInputCoro) const {
                /** Defines what happens when the coroutine is suspended. **/
                hInputCoro.resume();
            }

            void await_resume() const noexcept {
                /** Defines what happens when the coroutine is resumed **/
            }
        };


        struct promise_type
        {
            ConnectionStateCoroutine get_return_object()
            {
                std::println("[{}] [{}] promise_type::get_return_object()", tid(), time());
                return ConnectionStateCoroutine { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_never initial_suspend() const noexcept {
                return {};
            }

            std::suspend_never final_suspend() const noexcept {
                return {};
            }

            void return_void() const noexcept {

            }

            void unhandled_exception() const {
                std::terminate();
            }

            Awaiter<promise_type> await_transform(const std::string& strState) noexcept {
                return Awaiter<promise_type>(strState);
            }
        };

        std::coroutine_handle<promise_type> coroHandle {};

        explicit ConnectionStateCoroutine(const std::coroutine_handle<promise_type>& handle) : coroHandle { handle } {
            std::println("[{}] [{}] ConnectionStateCoroutine() created", tid(), time());
        }
    };

    ConnectionStateCoroutine manageConnection()
    {
        co_await "Connecting";
        co_await "Connected";
        co_await "Disconnecting";
        co_await "Disconnected";
    }
}


namespace State_Stored_in_Coroutine
{
    struct ConnectionStateCoroutine
    {
        template<class PromiseType>
        struct Awaiter
        {
            PromiseType& promise;

            explicit Awaiter(PromiseType& p, std::string s) : promise { p } {
                promise.state = std::move(s);
                std::println("[{}] [{}] \t Awaiter(promise.state = {}) created", tid(), time(), promise.state);
            }

            [[nodiscard]]
            bool await_ready() const noexcept
            {
                constexpr bool await_success = false;
                std::println("[{}] [{}] \t await_ready() --> {}", tid(), time(), await_success);
                return await_success;
            }

            void await_suspend(const std::coroutine_handle<PromiseType>& hInputCoro) const {
                /** Defines what happens when the coroutine is suspended. **/
                hInputCoro.resume();
            }

            [[nodiscard]]
            std::string await_resume() const noexcept
            {
                /** Defines what happens when the coroutine is resumed **/
                std::println("[{}] [{}] Awaiter::await_resume()", tid(), time());
                return promise.state;
            }
        };

        struct promise_type
        {
            std::string state { "Some_state" };

            ConnectionStateCoroutine get_return_object()
            {
                std::println("[{}] [{}] promise_type::get_return_object()", tid(), time());
                return ConnectionStateCoroutine { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_never initial_suspend() const noexcept {
                return {};
            }

            std::suspend_always final_suspend() const noexcept {
                return {};
            }

            void return_void() const noexcept {

            }

            void unhandled_exception() const {
                std::terminate();
            }

            Awaiter<promise_type> await_transform(const std::string& strState) noexcept {
                return Awaiter<promise_type>(*this, strState);
            }
        };

        std::coroutine_handle<promise_type> coroHandle {};

        explicit ConnectionStateCoroutine(const std::coroutine_handle<promise_type>& handle) : coroHandle { handle } {
            std::println("[{}] [{}] ConnectionStateCoroutine() created", tid(), time());
        }

        ~ConnectionStateCoroutine() noexcept
        {
            if (coroHandle) {
                coroHandle.destroy();
            }
            std::println("[{}] [{}] ConnectionStateCoroutine::~ConnectionStateCoroutine()", tid(), time());
        }
    };

    ConnectionStateCoroutine manageConnection()
    {
        std::cout << co_await "Connecting" << std::endl;
        std::cout << co_await "Connected" << std::endl;
        std::cout << co_await "Disconnecting" << std::endl;
        std::cout << co_await "Disconnected" << std::endl;
    }
}

void StdCoroutines::Experiments::State_Machine_Simple::    TestAll()
{
    // State_Stored_in_Awaitable::manageConnection();
    State_Stored_in_Coroutine::manageConnection();
}
