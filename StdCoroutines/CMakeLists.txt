cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 26)

project(StdCoroutines)
set(PROJECT_VERSION 1.0.0.0)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_compile_options(-c -Wall -O3 -std=c++26)

set(UTILS_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/Utilities")
set(UTILS_BINARY_DIR  "${CMAKE_BINARY_DIR}/Utilities")

include_directories(${THIRD_PARTY_DIR}/liburing/install_dir/include)
link_directories(${THIRD_PARTY_DIR}/liburing/install_dir/lib)

# message("CMAKE_SOURCE_DIR       : ${CMAKE_SOURCE_DIR}")
# message("UTILS_LIBRARY_DIR      : ${UTILS_LIBRARY_DIR}")
# message("PROJECT_SOURCE_DIR     : ${PROJECT_SOURCE_DIR}")
# message("CMAKE_BINARY_DIR       : ${CMAKE_BINARY_DIR}")
# message("CMAKE_CURRENT_LIST_DIR : ${CMAKE_CURRENT_LIST_DIR}")

add_executable(${PROJECT_NAME}
        main.cpp

        generators/Generators.cpp
        generators/Generic_Generator.cpp

        experiments/Experiments.cpp
        experiments/Calculating_Average.cpp
        experiments/PinBall_Game.cpp
        experiments/Generic_TaskBased_Coroutine.cpp
        experiments/FileReader.cpp
        experiments/FileReader_2.cpp
        experiments/TaskCoordination.cpp
        experiments/Event_Processor.cpp
        experiments/Event_Synchronization.cpp
        experiments/State_Machine_Simple.cpp
        experiments/Waitable_Coroutine_With_Mutex.cpp
        experiments/EventLoop_Simulation.cpp

        simple_examples/Coroutine_Lifecycle_CoReturn.cpp
        simple_examples/Coroutine_Lifecycle_CoAwait.cpp

        simple_examples/Resuming_Coroutine_1.cpp
        simple_examples/Returning_Coroutine.cpp
        simple_examples/Returning_Coroutine_2.cpp
        simple_examples/Returning_Coroutine_3.cpp
        simple_examples/Return_Resume_Lifecycle.cpp
        simple_examples/Return_Resume_Example.cpp
        simple_examples/Returning_Coroutine_ComputeResult.cpp

        simple_examples/Waitable_Coroutine.cpp
        simple_examples/Waitable_Coroutine_2.cpp
        simple_examples/Waitable_Coroutine_Update_Promise_State.cpp
        simple_examples/Multiple_Awaiters_Resolution.cpp
        simple_examples/Multiple_Awaiters_Resolution_2.cpp
        simple_examples/Mutiple_Awaiters_Initiail_Final.cpp
        simple_examples/Yield_Coroutine.cpp
        simple_examples/Yield_Coroutine_Values_from_List.cpp
        simple_examples/Awaiter_Lifecycle_Steps.cpp
        simple_examples/Awaiter_and_Awaitable.cpp
        simple_examples/Awaitable_Sleep.cpp

        networking/EpollCoroutine.cpp
        networking/EpollCoroutine_1_Ex.cpp
        networking/EpollCoroutine_2.cpp
        networking/EpollCoroutine_LessAlloc.cpp
        networking/URingCoroutine.cpp
        networking/URingCoroutine_2.cpp
        networking/Experiments.cpp
        networking/TcpClientEpoll.cpp
        networking/TcpClientEpoll_NoHeap.cpp
        networking/TcpClientEpoll_IOAwaiters.cpp
        networking/TcpClientEpoll_IOAwaiters_NoAlloc.cpp
        networking/TcpClientEpoll_Experimental.cpp

        threading/ThreadsHopping.cpp
        threading/ThreadPoolExecutor.cpp

        scheduling/SimpleExample.cpp

        exception_handling/Exception_Handling.cpp

        use_cases/Cooperative_Multitasking.cpp
        use_cases/FileReaderWithIterator.cpp

        frame_custom_allocation/CustomAllocation.cpp

        tests/Coroutine_Tests.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${UTILS_LIBRARY_DIR})
target_link_directories(${PROJECT_NAME} PUBLIC ${UTILS_BINARY_DIR})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        utils
        pthread
        uring
        ${EXTRA_LIBS}
)