/**============================================================================
Name        : Calculating_Average.cpp
Created on  : 27.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Calculating average
============================================================================**/

#include "Experiments.h"
#include <iostream>


namespace
{
    struct [[nodiscard]] Average
    {
        struct promise_type
        {
            Average get_return_object()
            {
                return Average { std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            std::suspend_always initial_suspend() {
                return std::suspend_always{};
            }

            void unhandled_exception() {
                std::terminate();
            }

            void return_value(auto valueIn) {
                value = valueIn;
            }

            std::suspend_always final_suspend() noexcept {
                return std::suspend_always{};
            }

            int value;
        };


        explicit Average(const std::coroutine_handle<promise_type>& handle) : coroHandle { handle } {
        }

        ~Average()
        {
            if (coroHandle) {
                coroHandle.destroy();
            }
        }
        Average(const Average&) = delete;
        Average& operator=(const Average&) = delete;

        [[nodiscard]]
        bool calculate() const
        {
            if (!coroHandle || coroHandle.done()) {
                return false; // we are done
            }
            coroHandle.resume();
            return !coroHandle.done();
        }

        [[nodiscard]]
        int getResult() const
        {
            return coroHandle.promise().value;
        }

    private:

        std::coroutine_handle<promise_type> coroHandle;
    };

    Average makeAverage(const std::vector<int>& numbers)
    {
        int sum {0};
        for (const auto& number : numbers)
        {
            std::cout << "Number crunching the next value"<< std::endl;
            sum += number;
            co_await std::suspend_always{};
        }

        std::cout << "Finally, all calculated." << std::endl;
        co_return sum / numbers.size();
    }

}


void StdCoroutines::Experiments::Calculating_Average::TestAll()
{
    const std::vector<int> numbers {100, 200, 100, 200, 100, 200, 100, 200};
    const Average average = makeAverage(numbers);

    while (average.calculate())
    {
        std::cout << "More calculations needed." << std::endl;
    }
    std::cout << "Average : " << average.getResult() << std::endl;
}
