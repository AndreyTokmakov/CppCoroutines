/**============================================================================
Name        : FileReader_2.cpp
Created on  : 28.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : FileReader_2
============================================================================**/

#include "Experiments.h"

#include <iostream>
#include <utility>
#include <fstream>
#include <filesystem>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '
#define ERR std::osyncstream { std::cerr } << Utilities::getCurrentTime() << ' '

namespace
{
    constexpr std::filesystem::path testDataDir() noexcept
    {
        return std::filesystem::current_path() / "../../data";
    }
}


namespace
{
    template<typename T>
    struct Generator
    {
        struct Promise;
        using promise_type = Promise;
        using value_type = T;

        struct Promise
        {
            value_type current_value;
            std::exception_ptr exception;

            Generator get_return_object() {
                return Generator {
                    std::coroutine_handle<Promise>::from_promise(*this)
                };
            }

            std::suspend_always initial_suspend() {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            template<typename U>
            std::suspend_always yield_value(U&& value) {
                current_value = std::forward<U>(value);
                return {};
            }

            void return_void() {
            }

            void unhandled_exception() {
                exception = std::current_exception();
            }
        };

        std::coroutine_handle<Promise> coroHandle;

        explicit Generator(std::coroutine_handle<Promise> hCoro) : coroHandle { hCoro } {
        }

        ~Generator()
        {
            if (coroHandle) {
                coroHandle.destroy();
            }
        }

        [[nodiscard]]
        bool next()
        {
            if (!coroHandle || coroHandle.done()) {
                return false;
            }
            coroHandle.resume();

            if (coroHandle.promise().exception) {
                std::rethrow_exception(coroHandle.promise().exception);
            }

            return !coroHandle.done();
        }

        [[nodiscard]]
        const value_type& value() const {
            return coroHandle.promise().current_value;
        }
    };

    Generator<std::vector<char>>
    readFileChunks(const std::filesystem::path& path, const std::size_t chunkSize)
    {
        std::fstream file(path);
        if (!file.is_open() || !file.good()) {
            co_return;
        }
        while (file)
        {
            std::vector<char> buffer(chunkSize);
            file.read(buffer.data(), chunkSize);
            const std::streamsize bytes_read = file.gcount();
            if (bytes_read <= 0)
                break;
            buffer.resize(bytes_read);
            co_yield buffer;
        }
    }

    void demo()
    {
        const std::filesystem::path file = testDataDir() / "file1.txt";
        Generator gen = readFileChunks(file, 10);
        while (gen.next())
        {
            auto chunk = gen.value();
            std::cout << "Chunk: ";
            for (char c : chunk)
                std::cout << c;
            std::cout << "\n";
        }
    }
}

void StdCoroutines::Experiments::FileReader_2::TestAll()
{
    demo();

    // Chunk: File_1_111
    // Chunk: File_1_222
    // Chunk: File_1_333
    // Chunk: File_1_444
    // Chunk: File_1_555
}
