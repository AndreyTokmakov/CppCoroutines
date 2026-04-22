/**============================================================================
Name        : Generators.h
Created on  : 05.04.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : SimpleCoroutines.h
============================================================================**/

#ifndef CPPCOROUTINES_GENERATORS_H
#define CPPCOROUTINES_GENERATORS_H

#include "Utilities.h"
#include <print>
#include <coroutine>


namespace StdCoroutines::Generators
{
    void TestAll();
    namespace Generic_Generator { void TestAll(); }
}

#endif //CPPCOROUTINES_GENERATORS_H
