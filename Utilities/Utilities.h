/**============================================================================
Name        : Utilities.h
Created on  : 01.03.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Utilities.h
============================================================================**/

#ifndef CPPPROJECTS_UTILITIES_H
#define CPPPROJECTS_UTILITIES_H

#include <chrono>

namespace Utilities
{
    [[nodiscard]]
    std::string getCurrentTime(const std::chrono::time_point<std::chrono::system_clock>& timestamp =
            std::chrono::system_clock::now());

}

#endif //CPPPROJECTS_UTILITIES_H
