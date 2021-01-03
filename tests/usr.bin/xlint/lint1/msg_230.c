/*	$NetBSD: msg_230.c,v 1.2 2021/01/03 15:35:01 rillig Exp $	*/
# 3 "msg_230.c"

// Test for message: nonportable character comparison, op %s [230]

/* lint1-flags: -S -g -p -w */

void example(char c, unsigned char uc, signed char sc)
{
	if (c < 0)
		if (uc < 0)
			if (sc < 0)
				return;

	/*
	 * XXX: The comparison "<= -1" looks very similar to "< 0",
	 * nevertheless "< 0" does not generate a warning.
	 *
	 * The comparisons may actually differ subtly because of the usual
	 * arithmetic promotions.
	 * */
	if (c <= -1)
		if (uc <= -1)
			if (sc <= -1)
				return;
}
