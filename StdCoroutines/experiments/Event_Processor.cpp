/**============================================================================
Name        : Event_Processor.cpp
Created on  : 16.03.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "Experiments.h"
#include <chrono>
#include <thread>
#include <queue>
#include <iostream>
#include <thread>
#include <fstream>
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
    struct Event
    {
        int id { 0 } ;
        std::string data;
    };

    struct EventQueue
    {
        std::queue<Event> events;

        void push(const Event& event) {
            events.push(event);
        }

        Event pop()
        {
            if (!events.empty()) {
                Event event = events.front();
                events.pop();
                return event;
            }
            return Event {-1, ""};
        }
    };

    struct EventHandlerCoro
    {
        template<class PromiseType>
        struct Awaiter
        {
            EventQueue& queue;

            explicit Awaiter(EventQueue& q): queue {q} {
                //std::println("[{}] [{}] \t Awaiter created", tid(), time());
            }

            ~Awaiter() {
                //std::println("[{}] [{}] \t Awaiter closed", tid(), time());
            }

            [[nodiscard]]
            bool await_ready() const
            {
                std::println("[{}] [{}] \t await_ready() --> {}", tid(), time(), !queue.events.empty());
                return !queue.events.empty();
            }

            void await_suspend(const std::coroutine_handle<PromiseType>& hInputCoro) const
            {
                /** Defines what happens when the coroutine is suspended. **/
                while (queue.events.empty()) {
                    // std::cout << "waiting for event ..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100u));
                }
                hInputCoro.resume();
            }

            Event await_resume()
            {
                /** Defines what happens when the coroutine is resumed **/
                return queue.pop();
            }
        };

        struct promise_type
        {
            EventHandlerCoro get_return_object()
            {
                std::println("[{}] [{}] promise_type::get_return_object()", tid(), time());
                return EventHandlerCoro { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_never initial_suspend() {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void return_void() {}
            void unhandled_exception() { std::terminate(); }

            Awaiter<promise_type> await_transform(EventQueue& queue) noexcept {
                return Awaiter<promise_type>(queue);
            }
        };

        std::coroutine_handle<promise_type> coroHandle;

        explicit EventHandlerCoro(const std::coroutine_handle<promise_type>& handle) : coroHandle { handle } {
            std::println("[{}] [{}] EventHandlerCoro() created", tid(), time());
        }
    };

    EventHandlerCoro handleEvents(EventQueue& queue)
    {
        while (true) {
            Event event = co_await queue;
            std::println("[{}] [{}] Handling Event({}, {})", tid(), time(), event.id, event.data);
        }
    }
}


void StdCoroutines::Experiments::Event_Processor::TestAll()
{
    EventQueue queue;
    std::thread eventProducer([&queue]() {
        for (int i = 0; i < 5; ++i) {
            queue.push({i, "EventData" + std::to_string(i)});
            std::this_thread::sleep_for(std::chrono::milliseconds(500u));
        }
    });

    handleEvents(queue);
    eventProducer.join();

}