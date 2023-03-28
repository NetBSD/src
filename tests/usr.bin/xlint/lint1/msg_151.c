/*	$NetBSD: msg_151.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_151.c"

// Test for message: void expressions may not be arguments, arg #%d [151]

/* lint1-extra-flags: -X 351 */

void sink_int(int);

void
example(int i)
{
	/* expect+1: error: void expressions may not be arguments, arg #1 [151] */
	sink_int((void)i);
	sink_int(i);
}
