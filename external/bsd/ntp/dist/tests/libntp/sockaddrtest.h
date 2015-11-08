/*	$NetBSD: sockaddrtest.h,v 1.2.4.3 2015/11/08 00:16:09 snj Exp $	*/

#ifndef TESTS_SOCKADDRTEST_H
#define TESTS_SOCKADDRTEST_H

#include "config.h"
#include "ntp.h"
#include "ntp_stdlib.h"


sockaddr_u CreateSockaddr4(const char* address, unsigned int port);

int IsEqual(const sockaddr_u expected, const sockaddr_u actual);

#endif // TESTS_SOCKADDRTEST_H
