/*	$NetBSD: modetoa.c,v 1.1.1.3 2015/10/23 17:47:45 christos Exp $	*/

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
