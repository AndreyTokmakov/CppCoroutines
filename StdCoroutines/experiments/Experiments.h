/**============================================================================
Name        : Experiments.h
Created on  : 05.04.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Experiments.h
============================================================================**/

#ifndef CPPCOROUTINES_EXPERIMENTS_H
#define CPPCOROUTINES_EXPERIMENTS_H

namespace StdCoroutines::Experiments
{
    void TestAll();
    namespace PinBall_Game { void TestAll(); };
    namespace Calculating_Average { void TestAll(); }
    namespace Waitable_Coroutine_With_Mutex { void TestAll(); }
    namespace Generic_TaskBased_Coroutine { void TestAll(); }
    namespace Event_Processor { void TestAll(); }
    namespace State_Machine_Simple { void TestAll();}
    namespace FileReader { void TestAll(); }
    namespace TaskCoordination { void TestAll(); }
}

#endif //CPPCOROUTINES_EXPERIMENTS_H
