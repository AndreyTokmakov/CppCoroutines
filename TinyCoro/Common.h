/**============================================================================
Name        : Common.h
Created on  : 29.02.2024
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : TinyCoro library tests
============================================================================**/

#ifndef CPPPROJECTS_COMMON_H
#define CPPPROJECTS_COMMON_H

#include <syncstream>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

constexpr std::osyncstream SyncOut(std::ostream& stream = std::cout)
{
    return std::osyncstream { stream };
}

typedef void (*funcPtr)(void*, int, int);

std::jthread AsyncCallbackAPI(void* userData, funcPtr cb, int i = 43)
{
    return std::jthread{[cb, userData, i] {
        SyncOut() << "  AsyncCallbackAPI... Thread id: " << std::this_thread::get_id() << '\n';
        std::this_thread::sleep_for(200ms);
        cb(userData, 42, i);
    }};
}

void AsyncCallbackAPIvoid(std::regular_invocable<void*, int> auto cb, void* userData)
{
    std::jthread t{[cb, userData] {
        SyncOut() << "  AsyncCallbackAPI... Thread id: " << std::this_thread::get_id() << '\n';
        std::this_thread::sleep_for(200ms);
        cb(userData, 42);
    }};
    t.detach();
}


#endif //CPPPROJECTS_COMMON_H
