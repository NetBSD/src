/*	$NetBSD: msg_151.c,v 1.3 2021/03/16 23:39:41 rillig Exp $	*/
# 3 "msg_151.c"

// Test for message: void expressions may not be arguments, arg #%d [151]

void sink_int(int);

void
example(int i)
{
	sink_int((void)i);	/* expect: 151 */
	sink_int(i);
}
