/**============================================================================
Name        : Threading.hpp
Created on  : 21.04.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Threading.hpp
============================================================================**/

#ifndef CPPCOROUTINES_THREADING_HPP
#define CPPCOROUTINES_THREADING_HPP

#include "Utilities.h"
#include <print>
#include <coroutine>

namespace StdCoroutines::Threading
{
    namespace ThreadsHopping { void TestAll(); }
    namespace ThreadPoolExecutor { void TestAll(); }
}

#endif //CPPCOROUTINES_THREADING_HPP