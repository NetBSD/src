/*	$NetBSD: msg_174.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_174.c"

// Test for message: too many initializers [174]

void
example(void)
{
	/* A single pair of braces is always allowed. */
	int n = { 13 };

	int too_many = { 17, 19 };	/* expect: 174 */

	/* XXX: Double-check with C99, this might be allowed. */
	int too_few = {};
}
