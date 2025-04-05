/**============================================================================
Name        : FileReader.cpp
Created on  : 28.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Simple Coroutine Waitable
============================================================================**/

#include "Experiments.h"
#include "Utilities.h"

#include <thread>
#include <fstream>

#include <coroutine>
#include <print>
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
    struct FileReader
    {
        struct promise_type;

        struct FileReadAwaiter
        {
            std::ifstream file;
            std::string filepath;
            std::string line;

            explicit FileReadAwaiter(const std::string& filename) : filepath { filename }
            {
                file.open(filepath);
                // log(filename);
                std::println("[{}] [{}] FileReadAwaiter::FileReadAwaiter({})", tid(), time(), filename);
            }

            ~FileReadAwaiter()
            {
                std::println("[{}] [{}] FileReadAwaiter::~FileReadAwaiter({})", tid(), time(), filepath);
            }

            [[nodiscard]]
            bool await_ready() noexcept
            {
                /** Called immediately before the coroutine is suspended
                 *  Allows as such, for some reason, to decide not to suspend after all
                 *  Returns true → coroutine is NOT suspended
                 *  Typically : return false;
                 *  Use case : suspension depends on some data availability
                **/
                std::println("[{}] [{}] FileReadAwaiter::await_ready({})", tid(), time(), filepath);
                return false;
            }

            void await_suspend(const std::coroutine_handle<promise_type>& coroHandle)
            {
                while (std::getline(file, line)) {
                    std::println("[{}] [{}] FileReadAwaiter::await_suspend(): {}", tid(), time(),line);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100u));
                }
                coroHandle.resume();
                std::println("[{}] [{}] FileReadAwaiter::await_suspend() done", tid(), time());
            }

            void await_resume() {
                std::println("[{}] [{}] FileReadAwaiter::await_resume({})", tid(), time(), filepath);
            }
        };


        struct promise_type
        {
            FileReader get_return_object()
            {
                std::println("[{}] [{}] promise_type::get_return_object()", tid(), time());
                return FileReader { std::coroutine_handle<promise_type>::from_promise(*this) };
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

            FileReadAwaiter await_transform(const std::string& filename) noexcept {
                return FileReadAwaiter { filename };
            }
        };

        std::coroutine_handle<promise_type> handle;
    };

    FileReader processFiles()
    {
        co_await R"(../../Coroutines/data/file1.txt)";
        co_await R"(../../Coroutines/data/file2.txt)";
    }
}


void StdCoroutines::Experiments::FileReader::TestAll()
{
    processFiles();
}
