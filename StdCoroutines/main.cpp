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

#include "generators/Generators.hpp"
#include "simple_examples/SimpleCoroutines.hpp"
#include "experiments/Experiments.h"
#include "exception_handling/Exception_Handling.hpp"
#include "tests/Coroutine_Tests.h"
#include "use_cases/Cooperative_Multitasking.hpp"
#include "frame_custom_allocation/CustomAllocation.hpp"
#include "networking/Networking.hpp"
#include "scheduling/ScheduleCoroutines.hpp"
#include "threading/Threading.hpp"


/**
+-------------------------------------------------------------------------+
|                                                                         |
|   1. A coroutine is a function that can SUSPEND and RESUME              |
|                                                                         |
|   2. It becomes a coroutine by using co_await, co_yield, or co_return   |
|                                                                         |
|   3. You must provide:                                                  |
|      * A return type (e.g., Generator<T>, Task<T>)                      |
|      * A nested promise_type inside that return type                    |
|      * The promise_type has hooks the compiler calls automatically      |
|                                                                         |
|   4. The compiler handles:                                              |
|      * Heap allocation of the coroutine frame                           |
|      * Saving/restoring state across suspensions                        |
|      * Calling your promise_type methods at the right times             |
|                                                                         |
|   5. coroutine_handle<promise_type> is your remote control:             |
|      * .resume()  --> continue running                                  |
|      * .done()    --> check if finished                                 |
|      * .destroy() --> free the frame                                    |
|      * .promise() --> access the promise object                         |
|                                                                         |
|   6. An "awaitable" has three methods:                                  |
|      * await_ready()   --> can we skip suspension?                      |
|      * await_suspend() --> what to do when suspending                   |
|      * await_resume()  --> what value does co_await produce             |
|                                                                         |
|   7. Common types:                                                      |
|      * Generator: uses co_yield, lazy, produces sequences               |
|      * Task: uses co_return, produces a single result                   |
|                                                                         |
|   8. Always use suspend_always for final_suspend(). Trust me.           |
|                                                                         |
|   9. Take parameters BY VALUE to avoid dangling references.             |
|                                                                         |
|  10. The standard gives you the engine. YOU build the car.              |
|      (Or use a library like cppcoro, libunifex, etc.)                   |
|                                                                         |
+-------------------------------------------------------------------------+

+-------------------------------------------------------------------------+
|                                                                         |
|  COROUTINE RETURN TYPE CHECKLIST                                        |
|                                                                         |
|  struct MyCoroutine {                                                   |
|                                                                         |
|      struct promise_type {            REQUIRED                          |
|                                       =======                           |
|          [ ] get_return_object()        Always needed                   |
|          [ ] initial_suspend()          Always needed (return awaitable)|
|          [ ] final_suspend() noexcept   Always needed (return awaitable)|
|          [ ] unhandled_exception()      Always needed                   |
|                                                                         |
|          Pick ONE:                    PICK ONE                          |
|          [ ] return_void()            ========                          |
|          [ ] return_value(T)          (depends on co_return usage)      |
|                                                                         |
|          Optional:                    OPTIONAL                          |
|          [ ] yield_value(T)           ========                          |
|                                       (only if using co_yield)          |
|          [ ] await_transform(expr)    (intercept co_await expressions)  |
|      };                                                                 |
|                                                                         |
|      [ ] coroutine_handle<promise_type> member                          |
|      [ ] Constructor from handle                                        |
|      [ ] Destructor calls handle.destroy()                              |
|      [ ] Delete copy, enable move                                       |
|      [ ] Public API for your use case (next, value, get, etc.)          |
|  };                                                                     |
|                                                                         |
+-------------------------------------------------------------------------+

**/
int main([[maybe_unused]] const int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    using namespace StdCoroutines;

    // Generators::TestAll();
    // Generators::Generic_Generator::TestAll();

    // Simple::Coroutine_Lifecycle_CoAwait::TestAll();
    // Simple::Coroutine_Lifecycle_CoReturn::TestAll();
    // Simple::Awaiter_Lifecycle_Steps::TestAll();

    // Simple::Returning_Coroutine::TestAll();
    // Simple::Returning_Coroutine_2::TestAll();
    // Simple::Returning_Coroutine_3::TestAll();
    // Simple::Return_Resume_Lifecycle::TestAll();
    // Simple::Return_Resume_Example::TestAll();
    // Simple::Returning_Coroutine_ComputeResult::TestAll();

    // Simple::Resuming_Coroutine_1::TestAll();
    // Simple::Awaitable_Sleep::TestAll();
    // Simple::Awaiter_and_Awaitable::TestAll();
    // Simple::Waitable_Coroutine::TestAll();
    // Simple::Waitable_Coroutine_2::TestAll();
    // Simple::Waitable_Coroutine_Update_Promise_State::TestAll();
    // Simple::Multiple_Awaiters_Resolution::TestAll();
    // Simple::Multiple_Awaiters_Resolution_2::TestAll();
    // Simple::Mutiple_Awaiters_Initiail_Final::TestAll();
    // Simple::Yield_Coroutine::TestAll();
    // Simple::Yield_Coroutine_Values_from_List::TestAll();

    // Experiments::TestAll();
    // Experiments::Waitable_Coroutine_With_Mutex::TestAll();
    // Experiments::Calculating_Average::TestAll();
    // Experiments::PinBall_Game::TestAll();
    // Experiments::Event_Processor::TestAll();
    Experiments::Event_Synchronization::TestAll();
    // Experiments::State_Machine_Simple::TestAll();
    // Experiments::Generic_TaskBased_Coroutine::TestAll();
    // Experiments::FileReader::TestAll();        // <------------- Not working
    // Experiments::FileReader_2::TestAll();        // <------------- Not working
    // Experiments::TaskCoordination::TestAll();  // <------------- Not working
    // Experiments::EventLoop_Simulation::TestAll();

    // Networking::EpollCoroutine::TestAll();
    // Networking::EpollCoroutine_1_Ex::TestAll();
    // Networking::EpollCoroutine_2::TestAll();
    // Networking::EpollCoroutine_LessAlloc::TestAll();
    // Networking::URingCoroutine::TestAll();
    // Networking::URingCoroutine_2::TestAll();
    // Networking::Experiments::TestAll();
    // Networking::TcpClientEpoll::TestAll();
    // Networking::TcpClientEpoll_2::TestAll();
    // Networking::TcpClientEpoll_IOAwaiters::TestAll();
    // Networking::TcpClientEpoll_IOAwaiters_NoAlloc::TestAll();
    //Networking::TcpClientEpoll_Experimental::TestAll();

    // Threading::ThreadsHopping::TestAll();
    Threading::ThreadPoolExecutor::TestAll();

    // ScheduleCoroutines::SimpleExample::TestAll();

    // Exception_Handling::TestAll();
    // custom_allocation::FrameAllocation::TestAll();
    // String_to_Integer_Parser::Test();
    // use_cases::cooperative_multitasking::TestAll();
    // CoroutineTests::TestAll();

    return EXIT_SUCCESS;
}

