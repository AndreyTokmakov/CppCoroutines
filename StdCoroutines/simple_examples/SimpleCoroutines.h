/**============================================================================
Name        : SimpleCoroutines.h
Created on  : 05.04.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : SimpleCoroutines.h
============================================================================**/

#ifndef CPPCOROUTINES_SIMPLECOROUTINES_H
#define CPPCOROUTINES_SIMPLECOROUTINES_H

#include "Utilities.h"
#include <print>
#include <coroutine>

namespace StdCoroutines::Simple
{
    namespace Coroutine_Lifecycle_CoAwait { void TestAll(); }
    namespace Coroutine_Lifecycle_CoReturn { void TestAll(); }
    namespace Returning_Coroutine { void TestAll(); }
    namespace Returning_Coroutine_2 { void TestAll(); }
    namespace Resuming_Coroutine_1 { void TestAll(); }
    namespace Awaiter_and_Awaitable { void TestAll(); }
    namespace Awaiter_Lifecycle_Steps { void TestAll(); }
    namespace Waitable_Coroutine { void TestAll(); }
    namespace Waitable_Coroutine_2 { void TestAll(); }
    namespace Waitable_Coroutine_Update_Promise_State { void TestAll(); }
    namespace Multiple_Awaiters_Resolution { void TestAll(); }
    namespace Multiple_Awaiters_Resolution_2 { void TestAll(); }
    namespace Mutiple_Awaiters_Initiail_Final { void TestAll(); }
    namespace Yield_Coroutine { void TestAll(); }
    namespace Yield_Coroutine_Values_from_List { void TestAll(); }
}

#endif //CPPCOROUTINES_SIMPLECOROUTINES_H
