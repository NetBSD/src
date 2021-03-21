/*	$NetBSD: msg_174.c,v 1.4 2021/03/21 10:43:08 rillig Exp $	*/
# 3 "msg_174.c"

// Test for message: too many initializers [174]

void
example(void)
{
	/* A single pair of braces is always allowed. */
	int n = { 13 };

	int too_many = { 17, 19 };	/* expect: 174 */

	/*
	 * An initializer list must have at least one expression, says the
	 * syntax definition in C99 6.7.8.
	 */
	int too_few = {};
}
