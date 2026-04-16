/**============================================================================
Name        : SimpleExample.cpp
Created on  : 16.04.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : ScheduleCoroutines.cpp
============================================================================**/

#include "ScheduleCoroutines.hpp"


#include "Utilities.h"
#include <memory>
#include <queue>
#include <utility>
#include <functional>


#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '


namespace
{
    using Utilities::getCurrentTime;
}

namespace
{
    struct Task
    {
        struct promise_type
        {
            Task get_return_object() {
                return Task{
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }

            std::suspend_never initial_suspend() {
                return {};
            }

            std::suspend_never final_suspend() noexcept {
                return {};
            }

            void return_void() {
            }

            void unhandled_exception() {
                std::terminate();
            }
        };

        std::coroutine_handle<promise_type> handle;
    };

    class Scheduler
    {
        std::queue<std::move_only_function<void()>> ready_queue_;

    public:
        static Scheduler& instance() {
            static Scheduler scheduler;
            return scheduler;
        }

        void schedule(std::move_only_function<void()> work) {
            ready_queue_.push(std::move(work));
        }

        void run()
        {
            while (!ready_queue_.empty())
            {
                std::move_only_function<void()> task = std::move(ready_queue_.front());
                ready_queue_.pop();
                LOG << "\t[Scheduler] Executing task\n";
                task();
            }
        }
    };

    // Awaitable that defers to the scheduler
    struct Defer
    {
        [[nodiscard]]
        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<Task::promise_type> hCoro)
        {
            LOG << "\t[Defer] await_suspend()\n";
            Scheduler::instance().schedule([hCoro]() {
                hCoro.resume();
            });
        }

        void await_resume() const noexcept {
        }
    };

    // Awaitable that owns a move-only resource
    struct AsyncQuery
    {
        std::unique_ptr<std::string> connection;

        [[nodiscard]]
        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<Task::promise_type> hCoro)
        {
            LOG << "\t[AsyncQuery] await_suspend()\n";
            std::unique_ptr<std::string> conn = std::move(connection);
            Scheduler::instance().schedule([hCoro, c = std::move(conn)]() mutable {
                LOG << "\t[db] queried via " << *c << "\n";
                hCoro.resume();
            });
        }

        void await_resume() const noexcept {
        }
    };

    Task do_work()
    {
        LOG << "1. starting work\n";
        co_await Defer{};
        LOG << "2. resumed after defer\n";

        auto conn = std::make_unique<std::string>("postgres://localhost/mydb");
        co_await AsyncQuery{std::move(conn)};
        LOG << "3. query complete\n";

        co_await Defer{};
        LOG << "4. all done\n";
    }

    void run()
    {
        do_work();
        Scheduler::instance().run();
    }
}

void ScheduleCoroutines::SimpleExample::TestAll()
{
    run();
    /**
    2026-04-16 19:20:11.372745 1. starting work
    2026-04-16 19:20:11.372818 	[Defer] await_suspend()
    2026-04-16 19:20:11.372825 	[Scheduler] Executing task
    2026-04-16 19:20:11.372830 2. resumed after defer
    2026-04-16 19:20:11.372833 	[AsyncQuery] await_suspend()
    2026-04-16 19:20:11.372836 	[Scheduler] Executing task
    2026-04-16 19:20:11.372841 	[db] queried via postgres://localhost/mydb
    2026-04-16 19:20:11.372845 3. query complete
    2026-04-16 19:20:11.372848 	[Defer] await_suspend()
    2026-04-16 19:20:11.372852 	[Scheduler] Executing task
    2026-04-16 19:20:11.372856 4. all done
    **/
}
