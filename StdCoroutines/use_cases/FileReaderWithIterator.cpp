/**============================================================================
Name        : FileReaderWithIterator.cpp
Created on  : 30.06.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : FileReaderWithIterator
============================================================================**/

#include "UseCases.hpp"

#include <vector>
#include <fstream>
#include <filesystem>
#include <string>
#include <print>
#include <utility>
#include <coroutine>
#include <iostream>


#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '
#define ERR std::osyncstream { std::cerr } << Utilities::getCurrentTime() << ' '

namespace
{
    using Path = std::filesystem::path;

    constexpr Path dataDir() noexcept {
        return std::filesystem::current_path() / "../../data";
    }
}


namespace
{
    /** The FileReader coroutine **/
    struct FileReader
    {
        struct Promise;
        struct Iterator;
        using promise_type = Promise;
        using coroutine_handle = std::coroutine_handle<Promise>;

        struct Promise
        {
            FileReader get_return_object() {
                return FileReader { coroutine_handle::from_promise(*this) };
            }

            std::suspend_always yield_value(std::string&& line) {
                currentLine = std::move(line);
                return std::suspend_always{};
            }

            std::suspend_always initial_suspend() const noexcept {
                return {};
            }

            std::suspend_always final_suspend() const noexcept {
                return {};
            }

            void unhandled_exception() {
                std::terminate();
            }

            void return_void() {}

        private:
            friend struct Iterator;
            std::string currentLine;
        };

        struct Iterator
        {
            coroutine_handle coroHandle;
            bool done { false };

            Iterator(const coroutine_handle& handle, const bool lastItem) :
                    coroHandle { handle }, done { lastItem }  {
            }

            Iterator& operator++()
            {
                coroHandle.resume();
                done = coroHandle.done();
                return *this;
            }

            // std::string operator*() const { return coroHandle.promise().currentLine; } // return value

            std::string& operator*() const {
                return coroHandle.promise().currentLine;
            }

            bool operator!=(std::default_sentinel_t) const noexcept {
                return !done;
            }
        };

        explicit FileReader(const coroutine_handle& handle) : coroHandle { handle } {
        }

        ~FileReader()
        {
            if (coroHandle) {
                coroHandle.destroy();
            }
        }

        [[nodiscard]]
        Iterator begin() const noexcept
        {
            coroHandle.resume();
            return Iterator { coroHandle, coroHandle.done() };
        }

        [[nodiscard]]
        std::default_sentinel_t end() const noexcept {
            return {};
        }

    private:

        coroutine_handle coroHandle;
    };

    FileReader readFile(const Path& filename)
    {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            co_yield std::move(line);
        }
    }
}

void use_cases::file_reader_with_iterator::TestAll()
{
    const Path filePath = dataDir() / "file2.txt";
    for (const std::string& line : readFile(filePath)) {
        std::println("{}", line);
    }
}
