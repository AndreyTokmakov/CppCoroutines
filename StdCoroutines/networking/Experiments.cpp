/**============================================================================
Name        : Experiments.cpp
Created on  : 18.02.2026
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Experiments.cpp
============================================================================**/

#include "Networking.hpp"

#include "Utilities.h"
#include <coroutine>
#include <utility>

#include <iostream>
#include <syncstream>
#include <print>
#include <array>

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <liburing.h>

#define LOG std::osyncstream { std::cout } << Utilities::getCurrentTime() << ' '

namespace
{

}

void StdCoroutines::Networking::Experiments::TestAll()
{
    // run();
}