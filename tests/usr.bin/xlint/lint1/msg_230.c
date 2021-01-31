/*	$NetBSD: msg_230.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_230.c"

// Test for message: nonportable character comparison, op %s [230]

/* lint1-flags: -S -g -p -w */

void example(char c, unsigned char uc, signed char sc)
{
	if (c < 0)
		if (uc < 0)			/* expect: 162 */
			if (sc < 0)
				return;

	/*
	 * XXX: The comparison "<= -1" looks very similar to "< 0",
	 * nevertheless "< 0" does not generate a warning.
	 *
	 * The comparisons may actually differ subtly because of the usual
	 * arithmetic promotions.
	 * */
	if (c <= -1)				/* expect: 230 */
		if (uc <= -1)			/* expect: 162 */
			if (sc <= -1)
				return;
}
