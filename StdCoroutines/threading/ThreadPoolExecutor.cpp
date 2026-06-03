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

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime()  \
     << " [" << std::this_thread::get_id() << "] "

namespace
{
    using namespace std::chrono;

    constexpr std::string_view FORMAT { "[%d-%02d-%02d %02d:%02d:%02d.%06ld] " };

    [[nodiscard]]
    std::string now(const time_point<system_clock>& timestamp = system_clock::now()) noexcept
    {
        const time_t time { system_clock::to_time_t(timestamp) };
        std::tm tm {};
        ::localtime_r(&time, &tm);

        std::string buffer(64, '\0');
        const int32_t size = std::sprintf(&(buffer.front()), FORMAT.data(),
                                          tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                                          duration_cast<microseconds>(timestamp - time_point_cast<seconds>(timestamp)).count());
        buffer.resize(size);
        buffer.shrink_to_fit();
        return buffer;
    }
}

namespace
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
                              const duration<Rep, Period> &timeout) noexcept
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


void StdCoroutines::Threading::ThreadPoolExecutor::TestAll()
{
    using RetType = std::string;

    auto func = [](const uint32_t timeout) -> RetType {
        std::this_thread::sleep_for(std::chrono::milliseconds(1u));
        LOG << "Starting job" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(timeout));
        LOG << "Job  done\n";
        return std::string("Task completed(timeout: " + std::to_string(timeout) + ")");
        // return 12345;
    };

    ThreadPool<RetType(int)> pool(4);
    std::vector<std::future<RetType>> results;
    for (int i = 1; i <= 8; i++)
    {
        results.push_back(pool.submit(func, 1));
    }

    LOG << "Waiting" << std::endl;
    for ( auto& F: results)
    {
        auto result = F.get();
        LOG<< result << std::endl;
    }
    LOG << "Jobs completed" << std::endl;

    const bool done = pool.stopSource.request_stop();
    LOG << "Done: " << std::boolalpha << done << std::endl;
};