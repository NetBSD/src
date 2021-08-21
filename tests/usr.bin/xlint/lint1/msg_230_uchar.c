/*	$NetBSD: msg_230_uchar.c,v 1.2 2021/08/21 11:50:57 rillig Exp $	*/
# 3 "msg_230_uchar.c"

// Test for message: nonportable character comparison, op %s [230]

/* lint1-flags: -S -g -p -w */
/* lint1-only-if: uchar */

void example(char c, unsigned char uc, signed char sc)
{
	/* expect+1: warning: comparison of char with 0, op < [162] */
	if (c < 0)
		return;
	/* expect+1: warning: comparison of unsigned char with 0, op < [162] */
	if (uc < 0)
		return;
	if (sc < 0)
		return;

	/*
	 * XXX: The comparison "<= -1" looks very similar to "< 0",
	 * nevertheless "< 0" does not generate a warning.
	 *
	 * The comparisons may actually differ subtly because of the usual
	 * arithmetic promotions.
	 * */
	/* expect+1: warning: nonportable character comparison, op <= [230] */
	if (c <= -1)
		return;
	/* expect+1: warning: comparison of unsigned char with negative constant, op <= [162] */
	if (uc <= -1)
		return;
	if (sc <= -1)
		return;
}
