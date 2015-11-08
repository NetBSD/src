/*	$NetBSD: modetoa.c,v 1.1.1.3.6.2 2015/11/08 00:16:09 snj Exp $	*/

#include "config.h"

#include "ntp_stdlib.h"

#include "unity.h"

void test_KnownMode(void);
void test_UnknownMode(void);


void
test_KnownMode(void) {
	const int MODE = 3; // Should be "client"

	TEST_ASSERT_EQUAL_STRING("client", modetoa(MODE));
}

void
test_UnknownMode(void) {
	const int MODE = 100;

	TEST_ASSERT_EQUAL_STRING("mode#100", modetoa(MODE));
}
