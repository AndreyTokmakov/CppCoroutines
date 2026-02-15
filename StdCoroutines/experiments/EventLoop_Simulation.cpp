/**============================================================================
Name        : EventLoop_Simulation.cpp
Created on  :
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Calculating average
============================================================================**/

#include "Experiments.h"

#include <iostream>
#include <queue>
#include <unordered_map>


namespace event_loop_simulation_example
{
    struct EventLoop
    {
        void schedule(std::coroutine_handle<> h) {
            readyQueue.push(h);
        }

        void run()
        {
            while (!readyQueue.empty())
            {
                std::coroutine_handle<> hCoro = readyQueue.front();
                readyQueue.pop();
                if (!hCoro.done())
                    hCoro.resume();
            }
        }

    private:
        std::queue<std::coroutine_handle<>> readyQueue;
    };

    EventLoop global_loop;


    struct Task
    {
        struct promise_type
        {
            using handle_t = std::coroutine_handle<promise_type>;

            Task get_return_object() {
                return Task { *this };
            }

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            // автоматически destroy после завершения
            struct FinalAwaiter
            {
                bool await_ready() noexcept {
                    return false;
                }

                void await_suspend(const handle_t handle) noexcept {
                    handle.destroy();
                }

                void await_resume() noexcept {
                }
            };

            FinalAwaiter final_suspend() noexcept {
                return {};
            }

            void return_void() noexcept {
            }

            void unhandled_exception() {
                std::terminate();
            }
        };

        using handle_t = std::coroutine_handle<promise_type>;

        explicit Task(promise_type& promise) :
            handle { std::coroutine_handle<promise_type>::from_promise(promise) } {
        }

        Task(Task&& other) noexcept : handle { std::exchange(other.handle, nullptr) } {
        }

        Task& operator=(Task&& other) noexcept {
            handle = std::exchange(other.handle, nullptr);
            return *this;
        }

        ~Task() = default; // destroy произойдёт в final_suspend

        handle_t handle;
    };

    // spawn (замена co_spawn)
    void spawn(Task task)
    {
        global_loop.schedule(task.handle);
        task.handle = nullptr; // ownership передан loop-у
    }

    template<typename T>
    struct Channel
    {
        void send(T value)
        {
            if (!awaiters.empty())
            {
                const std::coroutine_handle<> hCoro = awaiters.front();
                awaiters.pop();
                messages.push(std::move(value));
                global_loop.schedule(hCoro);
            }
            else
            {
                messages.push(std::move(value));
            }
        }

        struct Awaiter
        {
            Channel& channel;

            bool await_ready()
            {   /** Called immediately before the coroutine is suspended
                 *  Allows as such, for some reason, to decide not to suspend after all
                 *  Returns true → coroutine is NOT suspended
                 *  Typically : return false;
                 *  Use case : suspension depends on some data availability
                **/
                return not channel.messages.empty();
            }

            void await_suspend(const std::coroutine_handle<> handle)
            {   /** Called immediately after the coroutine is suspended
                 *  Will get called if await_ready() return False
                 *  Parameter: the handle of the coroutine that was suspended
                 *  In the body you can either return an other coroutine_handle type to change the call execution
                 *  Or you ca return nothing
                **/
                channel.awaiters.push(handle);
            }

            T await_resume()
            {   /** Called when the coroutine is resumed (after a successful suspension)
                 *  It is the final result of expression 'co_await ...'
                 *  It could return a value or nothing
                 *  Can return a value : The value the co_await expression yields
                **/
                T val = std::move(channel.messages.front());
                channel.messages.pop();
                return val;
            }
        };

        Awaiter receive() {
            return Awaiter{*this};
        }

    private:
        std::queue<T> messages;
        std::queue<std::coroutine_handle<>> awaiters;
    };

    struct Message
    {
        int client_id;
        std::string payload;
    };

    Channel<Message> router_in;

    // Пер-клиентская coroutine
    Task client_session(const int client_id)
    {
        std::cout << "[client " << client_id << "] started\n";
        while (true)
        {
            Message msg = co_await router_in.receive();
            if (msg.client_id != client_id)
                continue;
            std::cout << "[client " << client_id << "] recv: "<< msg.payload << "\n";
            if (msg.payload == "quit")
                break;
        }

        std::cout << "[client " << client_id << "] ended\n";
    }

    // Router coroutine
    Task router_loop()
    {
        std::unordered_map<int, bool> active_clients;
        while (true)
        {
            Message msg = co_await router_in.receive();
            if (!active_clients[msg.client_id]) {
                active_clients[msg.client_id] = true;
                spawn(client_session(msg.client_id));
            }

            // передаём сообщение дальше
            router_in.send(msg);
            if (msg.payload == "shutdown")
                break;
        }

        std::cout << "[router] shutdown\n";
    }

    void run()
    {
        spawn(router_loop());

        // имитация входящих сообщений
        router_in.send({1, "hello"});
        router_in.send({2, "ping"});
        router_in.send({1, "world"});
        router_in.send({2, "quit"});
        router_in.send({1, "quit"});
        router_in.send({0, "shutdown"});

        global_loop.run();
    }
}

void StdCoroutines::Experiments::EventLoop_Simulation::TestAll()
{
    event_loop_simulation_example::run();

    /**
    [router] shutdown
    [client 1] started
    [client 1] recv: hello
    [client 1] recv: world
    [client 1] recv: quit
    [client 1] ended
    [client 2] started
    [client 0] started
    **/
}
