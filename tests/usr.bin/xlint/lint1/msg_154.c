/*	$NetBSD: msg_154.c,v 1.3 2021/03/16 23:39:41 rillig Exp $	*/
# 3 "msg_154.c"

// Test for message: illegal combination of %s (%s) and %s (%s), arg #%d [154]

void sink_int(int);

void
example(int *ptr)
{
	sink_int(ptr);		/* expect: 154 */
}
