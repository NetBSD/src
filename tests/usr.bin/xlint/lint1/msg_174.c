/*	$NetBSD: msg_174.c,v 1.5 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_174.c"

// Test for message: too many initializers [174]

void
example(void)
{
	/* A single pair of braces is always allowed. */
	int n = { 13 };

	/* expect+1: error: too many initializers [174] */
	int too_many = { 17, 19 };

	/*
	 * An initializer list must have at least one expression, says the
	 * syntax definition in C99 6.7.8.
	 */
	int too_few = {};
}
