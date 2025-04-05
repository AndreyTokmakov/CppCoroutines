/**============================================================================
Name        : PinBall_Game.cpp
Created on  : 27.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description :
============================================================================**/

#include "Experiments.h"

namespace {
    using Utilities::getCurrentTime;
}

namespace
{
    /**
    Example: 2 – players – pinball
     – The caller is 1 player (I)
     – The co-routine is the other player (You)
     – We each take turns, we collaboratively work on the shared pinball machine
     – We want to win, so we will make the co-routine loose after N turns, aka the co-routine will be done

    The interface/api:
     – Resume method, let’s call it play()
     – No need for a get value method, since it returns/yields nothing
     – We configure N (turns) by specifying an integer to the co-routine invocation
    **/

    struct [[nodiscard]] Pinball
    {
        struct promise_type;

        explicit Pinball(const std::coroutine_handle<promise_type> handle) : coroHandle { handle } {
        }

        ~Pinball()
        {
            if (coroHandle)
            {
                coroHandle.destroy();
            }
        }

        Pinball(const Pinball&) = delete;
        Pinball& operator=(const Pinball&) = delete;

        [[nodiscard]]
        bool play() const
        {
            if (!coroHandle || coroHandle.done()) {
                return false; // we are done
            }
            coroHandle.resume();
            return !coroHandle.done();
        }

    private:

        std::coroutine_handle<promise_type> coroHandle;
    };

    struct Pinball::promise_type
    {
        Pinball get_return_object()
        {
            return Pinball { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        std::suspend_always initial_suspend()
        {
            return std::suspend_always{};
        }

        void unhandled_exception()
        {
            std::terminate();
        }

        void return_void()
        {
        }

        std::suspend_always final_suspend() noexcept
        {
            return std::suspend_always{};
        }
    };

    Pinball makePinball(const int turns)
    {
        for (int i = 0; i < turns; ++i)
        {
            std::println("[{}] Coroutine turn to play ({} attempt)", getCurrentTime(), i);
            co_await std::suspend_always{};
        }
        std::println("[{}] Done", getCurrentTime());
    }
}


void StdCoroutines::Experiments::PinBall_Game::TestAll()
{
    const Pinball pinBall = makePinball(4);
    while (pinBall.play())
    {
        std::println("[{}] My turn to play", getCurrentTime());
    }
}
