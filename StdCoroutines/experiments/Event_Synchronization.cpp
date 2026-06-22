/**============================================================================
Name        : Event_Synchronization.cpp
Created on  :
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Event_Synchronization
============================================================================**/

#include "Experiments.h"

#include <iostream>
#include <print>
#include <atomic>
#include <syncstream>
#include <thread>
#include <stdexcept>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime()  \
    << " [" << std::this_thread::get_id() << "] "


namespace events_synch
{
    struct Event
    {
        Event() = default;
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;

        // forward declare
        struct  Awaiter;

        // returns the awaiter
        Awaiter operator co_await() const noexcept;

        // sender calls this
        void notify() noexcept;


    private:

        friend struct Awaiter;

        // Who's waiting? nullptr if nobody.
        // Points to the Awaiter (which holds the coroutine handle).
        mutable std::atomic<void*> suspendedWaiter { nullptr };

        // Has notify() been called? This is the "memory" that prevents lost wakeups.
        // Once set to true, it stays true.
        mutable std::atomic<bool> notified { false };
    };


    struct Event::Awaiter
    {
    public:
        explicit Awaiter(const Event& eve) : event(eve) {
        }

        // STEP 1: Should we suspend?
        //         Called FIRST when co_await is evaluated.
        //         If the event is already notified → return true → don't suspend.
        //         This handles the "notify before wait" case.
        bool await_ready() const
        {
            // Only one waiter allowed (simplification).
            if (event.suspendedWaiter.load() != nullptr) {
                throw std::runtime_error("Only one waiter allowed");
            }

            // If already notified, don't suspend. Run straight through.
            // This is what PREVENTS LOST WAKEUPS.
            return event.notified.load();
        }

        // STEP 2: We're suspending. Store the handle.
        //         Called only if await_ready() returned false.
        //         The compiler passes us the coroutine_handle - that's our "resume ticket."
        //         We store it so notify() can use it later.
        // Returns true  → "yes, really suspend"
        // Returns false → "changed my mind, don't suspend"
        //                  (race condition: notified between ready and suspend)
        bool await_suspend(const std::coroutine_handle<> handle) noexcept
        {
            coroutineHandle = handle;

            // Double-check: did notify() sneak in after await_ready()?
            if (event.notified.load()) {
                return false; // --> don't suspend
            }
            // Store ourselves so notify() can find us.
            event.suspendedWaiter.store(this);
            return true;  // suspend for real
        }

        // STEP 3: We've been resumed.
        //         Called after handle.resume() brings us back.
        void await_resume() noexcept
        {
            // Nothing to return for this simple Event.
        }

    private:
        friend struct Event;
        const Event& event;
        std::coroutine_handle<> coroutineHandle;
    };


    void Event::notify() noexcept
    {   // Set the flag FIRST. This is the "memory" that prevents lost wakeups.
        // Even if nobody is waiting yet, the flag  persists.
        // When they eventually co_await, await_ready() sees notified==true and skips suspension entirely.
        notified.store(true);

        // Is anyone currently suspended and waiting?
        if (const auto* waiter = static_cast<Awaiter*>(suspendedWaiter.load()); waiter != nullptr)
        {   // Yes - resume their coroutine.
            // This is the equivalent of condition_variable::notify_one(), except it's  DIRECT.
            // No spurious wakeups -->  No broadcast/
            // Just "wake up this specific coroutine."
            waiter->coroutineHandle.resume();
        }
    }

    Event::Awaiter Event::operator co_await() const noexcept {
        return Awaiter{*this};
    }

    struct [[nodiscard]] Task
    {
        struct promise_type
        {
            Task get_return_object()
            {
                return Task { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_never initial_suspend() {
                return std::suspend_never{};
            }

            std::suspend_never final_suspend() noexcept {
                return std::suspend_never{};
            }

            void unhandled_exception() {
                std::terminate();
            }

            void return_void() {
            }
        };


        explicit Task(const std::coroutine_handle<promise_type>& handle) : coroHandle { handle } {
        }

        /* --- Crushing with it
        ~Task()
        {
            if (coroHandle) {
                coroHandle.destroy();
            }
        }
        */

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

    private:

        std::coroutine_handle<promise_type> coroHandle;
    };

    // The coroutine
    Task receiver(Event& event)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        co_await event;
        const auto elapsed = std::chrono::high_resolution_clock::now() - start;
        LOG << "Got it! Waited " << std::chrono::duration<double>(elapsed).count() << " seconds.\n";
    }
}


void StdCoroutines::Experiments::Event_Synchronization::TestAll()
{
    using namespace events_synch;
    using namespace std::chrono_literals;

    std::cout << std::string(120, '-') << "\n\t\t Case 1: notify BEFORE co_await\n"
            << std::string(120, '-') << std::endl;
    {
        Event event;
        std::thread sender([&] { event.notify(); });
        std::thread recv(receiver, std::ref(event));
        sender.join();
        recv.join();
    }

    std::cout << std::string(120, '-') << "\n\t\t Case 2: notify AFTER 2s\n"
            << std::string(120, '-') << std::endl;
    {
        Event event;
        std::thread recv(receiver, std::ref(event));
        std::thread sender([&] {
            std::this_thread::sleep_for(2s);
            event.notify();
        });
        recv.join();
        sender.join();
    }
}