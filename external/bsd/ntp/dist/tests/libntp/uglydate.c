/*	$NetBSD: uglydate.c,v 1.1.1.3.6.3 2016/05/08 21:51:05 snj Exp $	*/

#include "config.h"

#include "ntp_stdlib.h"
#include "ntp_fp.h"

#include "unity.h"

void setUp(void);
void test_ConstantDateTime(void);


void
setUp(void)
{
	init_lib();

	return;
}

void
test_ConstantDateTime(void)
{
	const u_int32 HALF = 2147483648UL;

	l_fp e_time = {{3485080800UL}, HALF}; /* 2010-06-09 14:00:00.5 */

	TEST_ASSERT_EQUAL_STRING("3485080800.500000 10:159:14:00:00.500",
				 uglydate(&e_time));
	return;
}
