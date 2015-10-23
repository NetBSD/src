/*	$NetBSD: uglydate.c,v 1.1.1.3 2015/10/23 17:47:45 christos Exp $	*/

#include "config.h"

#include "ntp_stdlib.h"
#include "ntp_fp.h"

#include "unity.h"

void test_ConstantDateTime(void);

void
test_ConstantDateTime(void) {
	const u_int32 HALF = 2147483648UL;

	l_fp time = {{3485080800UL}, HALF}; /* 2010-06-09 14:00:00.5 */

	TEST_ASSERT_EQUAL_STRING("3485080800.500000 10:159:14:00:00.500",
				 uglydate(&time));
}
