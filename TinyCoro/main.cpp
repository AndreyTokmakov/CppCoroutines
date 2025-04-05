/**============================================================================
Name        : main.cpp
Created on  : 20.02.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Coroutines
============================================================================**/

#include <iostream>
#include <vector>
#include <string_view>
#include "TinyCoro.h"

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    TinyCoro::TestAll();

    return EXIT_SUCCESS;
}

