/**============================================================================
Name        : main.cpp
Created on  : 05.04.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Coroutines
============================================================================**/

#include <iostream>
#include <vector>
#include <string_view>

#include "generators/Generators.h"
#include "simple_examples/SimpleCoroutines.h"
#include "experiments/Experiments.h"


int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    using namespace StdCoroutines;


    // Simple::Coroutine_Lifecycle_CoAwait::TestAll();
    // Simple::Coroutine_Lifecycle_CoReturn::TestAll();

    // Simple::Awaiter_Lifecycle_Steps::TestAll();

    // Simple::Returning_Coroutine::TestAll();
    // Simple::Returning_Coroutine_2::TestAll();

    // Simple::Resuming_Coroutine_1::TestAll();

    // Simple::Awaiter_and_Awaitable::TestAll();
    // Simple::Waitable_Coroutine::TestAll();
    // Simple::Waitable_Coroutine_2::TestAll();
    Simple::Waitable_Coroutine_Update_Promise_State::TestAll();

    // Simple::Yield_Coroutine::TestAll();
    // Simple::Yield_Coroutine_Values_from_List::TestAll();

    // Simple::Multiple_Awaiters_Resolution::TestAll();
    // Simple::Multiple_Awaiters_Resolution_2::TestAll();

    // Generators::TestAll();

    // Experiments::TestAll();
    // Experiments::Waitable_Coroutine_With_Mutex::TestAll();
    // Experiments::Calculating_Average::TestAll();
    // Experiments::PinBall_Game::TestAll();
    // Experiments::Event_Processor::TestAll();
    // Experiments::State_Machine_Simple::TestAll();
    // Experiments::Generic_TaskBased_Coroutine::TestAll();
    // Experiments::FileReader::TestAll();        // <------------- Not working
    // Experiments::TaskCoordination::TestAll();     // <------------- Not working

    // String_to_Integer_Parser::Test();

    return EXIT_SUCCESS;
}

