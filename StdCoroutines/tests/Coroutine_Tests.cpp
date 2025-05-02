/**============================================================================
Name        : Coroutine_Tests.cpp
Created on  : 06.04.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Coroutine_Tests.cpp
============================================================================**/

#include "Coroutine_Tests.h"

#include <thread>
#include <iostream>

#if 0
namespace
{
    auto tid() { return std::this_thread::get_id();}
    auto time() { return Utilities::getCurrentTime();}


    template<typename PromiseType>
    struct Awaiter
    {
        PromiseType& promise;

        explicit Awaiter(PromiseType& inPromise): promise { inPromise }
        {
            //std::println("[{}] [{}] Awaiter::Awaiter({})", tid(), time());
        }

        /** await_ready():
         *  This is called to check whether the coroutine is suspended.
         *  If that is the case,it returns false.
         *  In our example, it always returns true to indicate the coroutine is not suspended.
        **/
        bool await_ready() noexcept
        {
            std::println("[{}] [{}] Awaiter::await_ready()", tid(), time());
            return false; /** TRUE ==> await_resume(), FALSE ==> await_suspend() **/
        }

        bool await_suspend(const std::coroutine_handle<PromiseType>& inCoroHandle) noexcept
        {
            promise = inCoroHandle.promise();
            return false;     // says no don't suspend coroutine after all
        }

        /** await_resume():
         *  This resumes the coroutine and generates the result of the co_await expression.
         **/
        PromiseType& await_resume() noexcept
        {
            std::println("[{}] [{}] Awaiter::await_resume()", tid(), time());
            return promise;
        }
    };


    struct CoroutinesTask
    {
        struct Promise
        {
            int counter { 0 };

            CoroutinesTask get_return_object() noexcept
            {
                // std::println("[{}] [{}] \t Promise::get_return_object()", tid(), time());
                return CoroutinesTask {std::coroutine_handle<Promise>::from_promise(*this)};
            }

            void return_void() noexcept {
                std::println("[{}] [{}] \t\t Promise::return_void()", tid(), time());
            }

            std::suspend_never initial_suspend() noexcept
            {
                std::println("[{}] [{}] \t\t Promise::initial_suspend()", tid(), time());
                return {};
            }

            std::suspend_never final_suspend() noexcept
            {
                std::println("[{}] [{}] \t\t Promise::final_suspend()", tid(), time());
                return {};
            }

            void unhandled_exception() {
                std::println("[{}] [{}] \t\t Promise::unhandled_exception()", tid(), time());
                std::terminate();
            }

            /**
             * when the coroutine calls co_await, the compiler will generate code to call a function in the promise
             * object called await_transform(), which has a parameter of the same type as the data the coroutine is waiting for.
             *
             * As its name implies, await_transform is a function that transforms any object (in our example, std::string)
             * into an awaitable object.
             *
             * std::string is not awaitable, hence the previous compiler error. await_transform() must return an awaiter object.
             * his is just a simple struct implementing a required interface for the awaiter to be usable by the compiler.
            **/

            auto await_transform(int value) noexcept
            {
                std::println("[{}] [{}] CoroutinesTask::promise_type::await_transform()", tid(), time());
                return Awaiter<CoroutinesTask::Promise>(*this);
            }
        };

        using promise_type = Promise;
        std::coroutine_handle<promise_type> handle {};

        explicit CoroutinesTask(const std::coroutine_handle<promise_type>& handle) : handle { handle }
        {
            std::println("[{}] [{}] CoroutinesTask::CoroutinesTask()", tid(), time());
        }

        ~CoroutinesTask() noexcept
        {
            if (handle) {
                handle.destroy();
            }
            std::println("[{}] [{}] CoroutinesTask::~CoroutinesTask()", tid(), time());
        }

        /*void putValue(std::string msg) const noexcept
        {
            handle.promise().input_data = std::move(msg);
            if (!handle.done()) {
                handle.resume();
            }
        }*/

        CoroutinesTask createCoroutine()
        {
            // auto promise = co_await 123;
            auto promise = co_await Awaiter<CoroutinesTask::Promise>{};

            for (int i = 0;; ++i) {
                promise.counter = i;
                co_await std::suspend_always{};
            }
        }
    };
}
#endif

namespace
{
    template<typename PromiseType>
    struct GetPromise
    {
        PromiseType *ptrPromise;

        bool await_ready() {
            return false;
        }

        bool await_suspend(std::coroutine_handle<PromiseType> h) {
            ptrPromise = &h.promise();
            return false;     // says no don't suspend coroutine after all
        }
        PromiseType *await_resume() {
            return ptrPromise;
        }
    };

    struct CoroutinesTask
    {
        struct Promise
        {
            int value { 0 };

            CoroutinesTask get_return_object() {
                return CoroutinesTask {std::coroutine_handle<Promise>::from_promise(*this)};
            }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void unhandled_exception() {}
        };

        using promise_type = Promise;
        std::coroutine_handle<promise_type> handle {};

        operator std::coroutine_handle<Promise>() const {
            return handle;
        }
    };

    CoroutinesTask getCounter()
    {
        auto pp = co_await GetPromise<CoroutinesTask::Promise>{};
        for (unsigned i = 0;; ++i) {
            pp->value = i;
            co_await std::suspend_always{};
        }
    }
}

void StdCoroutines::CoroutineTests::TestAll()
{
    std::coroutine_handle<CoroutinesTask::Promise> h = getCounter();
    CoroutinesTask::Promise &promise = h.promise();
    for (int i = 0; i < 3; ++i) {
        std::cout << "counter: " << promise.value << std::endl;
        h();
    }
    h.destroy();
}