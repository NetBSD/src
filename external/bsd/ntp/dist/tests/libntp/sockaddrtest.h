/*	$NetBSD: sockaddrtest.h,v 1.1.1.8 2020/05/25 20:40:16 christos Exp $	*/

#ifndef TESTS_SOCKADDRTEST_H
#define TESTS_SOCKADDRTEST_H

#include "config.h"
#include "ntp.h"
#include "ntp_stdlib.h"


sockaddr_u CreateSockaddr4(const char* address, unsigned int port);

int IsEqual(const sockaddr_u expected, const sockaddr_u actual);

#endif // TESTS_SOCKADDRTEST_H
