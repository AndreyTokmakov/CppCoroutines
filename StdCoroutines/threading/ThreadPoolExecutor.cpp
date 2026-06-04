/**============================================================================
Name        : ThreadPoolExecutor.cpp
Created on  : 10.05.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : ThreadPoolEx.cpp
============================================================================**/

#include "Threading.hpp"

#include <iostream>
#include <future>
#include <chrono>
#include <functional>
#include <thread>
#include <string>
#include <vector>
#include <deque>
#include <syncstream>
#include <utility>


#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime()  \
     << " [" << std::this_thread::get_id() << "] "

namespace thread_pool_executor
{
    template<typename ReturnType,
            typename ... Args>
    class TaskContext
    {
        using Callable = std::packaged_task<ReturnType (Args...)>;
        using Params   = std::tuple<Args...>;
        using Indices  = std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Params>>>;

        Callable func;
        Params params;

    private:

        template<typename Func,
                typename TupleType,
                size_t... Indices>
        constexpr void invokeTask(Func&& task,
                                  TupleType&& tup,
                                  std::index_sequence<Indices...>)
        {
            std::invoke(task, std::get<Indices>(std::forward<TupleType>(tup))...);
        }

    public:

        using Future   = std::future<ReturnType>;

    public:

        TaskContext() = default;

        template<typename Func, typename ... ParamTypes>
        explicit TaskContext(Func&& task, ParamTypes&& ... params):
                func { std::forward<Func>(task) }, params { std::forward<ParamTypes>(params)... } {
        }

        TaskContext(const TaskContext& ctx) = delete;
        TaskContext& operator=(const TaskContext& ctx) = delete;

        TaskContext(TaskContext&& ctx) noexcept  = default;
        TaskContext& operator=(TaskContext&& ctx) noexcept = default;

        [[nodiscard]]
        Future getFuture() noexcept {
            return func.get_future();
        }

        void invoke()
        {
            constexpr std::integer_sequence idxSequence = Indices {};
            invokeTask(func,std::forward<Params>(params), idxSequence);
        }
    };


    template<typename T>
    struct ThreadPool;

    template<typename ReturnType, typename ... Args>
    struct ThreadPool<ReturnType (Args...)>
    {
        using Task   = TaskContext<ReturnType, Args...>;
        using Future = Task::Future;

        mutable std::mutex mutex;
        std::condition_variable taskAdded;
        std::deque<Task> taskQueue;

        std::vector<std::jthread> workers {};
        std::stop_source stopSource;

        static inline constexpr std::chrono::duration<uint64_t, std::ratio<1, 1000>> pollTimeout {
                std::chrono::milliseconds(500u)
        };

    private:

        template<class Rep, class Period>
        bool wait_for_and_pop(Task& task,
                              const std::chrono::duration<Rep, Period> &timeout) noexcept
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (!taskAdded.wait_for(lock, timeout, [this] { return !taskQueue.empty();}))
                return false;

            task = std::move(taskQueue.front());
            taskQueue.pop_front();
            return true;
        }

        // TODO: Try-catch ???
        void executor(const std::stop_source& source)
        {
            Task task;
            while (!source.stop_requested()) {
                if (const bool result = wait_for_and_pop(task, pollTimeout); result) {
                    task.invoke();
                }
            }
        }

    public:

        explicit ThreadPool(const uint32_t threadsCount = std::thread::hardware_concurrency())
        {
            workers.reserve(threadsCount);
            for (size_t i = 0; i < threadsCount; ++i) {
                workers.emplace_back(&ThreadPool::executor, this, stopSource);
            }
        }

        [[nodiscard("Its not for free")]]
        bool empty() const noexcept
        {
            std::lock_guard<std::mutex> lock(mutex);
            return taskQueue.empty();
        }

        [[nodiscard("Its not for free")]]
        size_t size() const noexcept
        {
            std::lock_guard<std::mutex> lock(mutex);
            return taskQueue.size();
        }

        template<typename Func, typename ... _Args>
        Future submit(Func&& task, _Args&& ... params) noexcept
        {
            std::unique_lock<std::mutex> lock { mutex };
            Future futureResult = taskQueue.emplace_back(std::forward<Func>(task),
                                                         std::forward<_Args>(params)... ).getFuture();
            lock.unlock();
            taskAdded.notify_all(); // TODO:  one / all ?
            return futureResult;
        }
    };
}


namespace thread_pool_executor::coroutine
{
    ThreadPool<void()> coroThreadPool { 2 };

    struct Task
    {
        struct Promise;
        using promise_type = Promise;

        struct Promise
        {
            Task get_return_object() {
                return Task{
                    std::coroutine_handle<Promise>::from_promise(*this)
                };
            }

            std::suspend_never initial_suspend() noexcept {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void return_void() {
            }

            void unhandled_exception(){
                std::terminate();
            }
        };

        std::coroutine_handle<promise_type> handle;

        explicit Task(const std::coroutine_handle<Promise> hCoro) : handle(hCoro) {
        }

        ~Task()
        {
            if (handle) {
                handle.destroy();
            }
        }

        Task(Task&& other) noexcept : handle { std::exchange(other.handle, nullptr)} {
        }
        Task(const Task&) = delete;
    };

    struct PoolExecutorAwaitable
    {
        bool await_ready() noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> hCoro) noexcept
        {
            coroThreadPool.submit([hCoro]() {
                hCoro.resume();
            });
        }

        void await_resume() noexcept {
        }
    };
}


namespace thread_pool_executor::tests
{
    using namespace std::chrono_literals;
    using namespace thread_pool_executor::coroutine;

    void threadPoolTests()
    {
        auto func = [](const uint32_t timeout) -> std::string {
            std::this_thread::sleep_for(1ms);
            LOG << "Starting job\n";
            std::this_thread::sleep_for(std::chrono::seconds(timeout));
            LOG << "Job done\n";
            return std::string("Task completed(timeout: " + std::to_string(timeout) + ")");
        };

        ThreadPool<std::string(int)> pool(2);
        std::vector<std::future<std::string>> results;
        for (int i = 1; i <= 4; i++) {
            results.push_back(pool.submit(func, 1));
            LOG << "Task submitted\n";
        }

        std::this_thread::sleep_for(3s);

        const bool done = pool.stopSource.request_stop();
        LOG << "Done: " << std::boolalpha << done << std::endl;
    }

    Task demo()
    {
        for (int i = 0; i < 5; i++) {
            LOG << "Resuming coroutine using ThreadPool\n";
            co_await PoolExecutorAwaitable{};
        }
    }

    void runCoroutineTest()
    {
        LOG << "Thread\n";
        Task task = demo();
        std::this_thread::sleep_for(500ms);

        const bool done = coroThreadPool.stopSource.request_stop();
        LOG << "Done: " << std::boolalpha << done << std::endl;

        // 2026-06-04 18:49:13.218327 [140149885028160] Thread
        // 2026-06-04 18:49:13.218400 [140149885028160] Resuming coroutine using ThreadPool
        // 2026-06-04 18:49:13.218414 [140149885024000] Resuming coroutine using ThreadPool
        // 2026-06-04 18:49:13.218458 [140149885024000] Resuming coroutine using ThreadPool
        // 2026-06-04 18:49:13.218463 [140149885024000] Resuming coroutine using ThreadPool
        // 2026-06-04 18:49:13.218467 [140149885024000] Resuming coroutine using ThreadPool
        // 2026-06-04 18:49:13.718492 [140149885028160] Done: true
    }

}

void StdCoroutines::Threading::ThreadPoolExecutor::TestAll()
{
    using namespace thread_pool_executor::tests;

    // threadPoolTests();
    runCoroutineTest();
}