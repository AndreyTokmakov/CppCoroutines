/**============================================================================
Name        : Networking.hpp
Created on  : 15.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Networking.hpp
============================================================================**/

#ifndef CPPCOROUTINES_NETWORKING_HPP
#define CPPCOROUTINES_NETWORKING_HPP

namespace StdCoroutines::Networking
{
    namespace EpollCoroutine { void TestAll(); }
    namespace EpollCoroutine_1_Ex { void TestAll(); }
    namespace EpollCoroutine_2 { void TestAll(); }
    namespace EpollCoroutine_LessAlloc { void TestAll(); }
    namespace URingCoroutine { void TestAll(); }
    namespace URingCoroutine_2 { void TestAll(); }
    namespace Experiments { void TestAll(); }

    namespace TcpClientEpoll { void TestAll(); }
    namespace TcpClientEpoll_2 { void TestAll(); }
    namespace TcpClientEpoll_IOAwaiters { void TestAll(); }
}

#endif //CPPCOROUTINES_NETWORKING_HPP