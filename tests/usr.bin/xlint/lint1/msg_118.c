/*	$NetBSD: msg_118.c,v 1.6 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_118.c"

/* Test for message: semantics of '%s' change in ANSI C; use explicit cast [118] */

/* lint1-flags: -hw */

int
int_shl_uint(int left, unsigned int right)
{
	return left << right;
}

int
int_shr_uint(int left, unsigned int right)
{
	/* expect+1: warning: semantics of '>>' change in ANSI C; use explicit cast [118] */
	return left >> right;
}

int
int_shl_int(int left, int right)
{
	return left << right;
}

/*
 * The behavior of typeok_shl can only be triggered on 64-bit platforms, or
 * in C99 mode, and the above tests require C90 mode.
 *
 * On 32-bit platforms both operands of the '<<' operator are first promoted
 * individually, and since C90 does not know 'long long', the maximum
 * bit-size for an integer type is 32 bits.
 *
 * Because of this difference there is no test for that case since as of
 * 2021-05-04, all lint1 tests must be independent of the target platform and
 * there is no way to toggle individual tests depending on the properties of
 * the target platform.
 */
