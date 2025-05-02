cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 26)

project(StdCoroutines)
set(PROJECT_VERSION 1.0.0.0)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_compile_options(-c -Wall -O3 -std=c++26)

set(UTILS_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/Utilities")
set(UTILS_BINARY_DIR  "${CMAKE_BINARY_DIR}/Utilities")

# message("CMAKE_SOURCE_DIR       : ${CMAKE_SOURCE_DIR}")
# message("UTILS_LIBRARY_DIR      : ${UTILS_LIBRARY_DIR}")
# message("PROJECT_SOURCE_DIR     : ${PROJECT_SOURCE_DIR}")
# message("CMAKE_BINARY_DIR       : ${CMAKE_BINARY_DIR}")
# message("CMAKE_CURRENT_LIST_DIR : ${CMAKE_CURRENT_LIST_DIR}")

add_executable(${PROJECT_NAME}
        main.cpp

        generators/Generators.cpp

        experiments/Experiments.cpp
        experiments/Calculating_Average.cpp
        experiments/PinBall_Game.cpp
        experiments/Generic_TaskBased_Coroutine.cpp
        experiments/FileReader.cpp
        experiments/TaskCoordination.cpp
        experiments/Event_Processor.cpp
        experiments/State_Machine_Simple.cpp
        experiments/Waitable_Coroutine_With_Mutex.cpp

        simple_examples/Coroutine_Lifecycle_CoReturn.cpp
        simple_examples/Coroutine_Lifecycle_CoAwait.cpp
        simple_examples/Resuming_Coroutine_1.cpp
        simple_examples/Returning_Coroutine.cpp
        simple_examples/Returning_Coroutine_2.cpp
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

        tests/Coroutine_Tests.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${UTILS_LIBRARY_DIR})
target_link_directories(${PROJECT_NAME} PUBLIC ${UTILS_BINARY_DIR})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        utils
        pthread
        ${EXTRA_LIBS}
)