/*	$NetBSD: msg_154.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_154.c"

// Test for message: illegal combination of %s (%s) and %s (%s), arg #%d [154]

void sink_int(int);

void
example(int *ptr)
{
	/* expect+1: warning: illegal combination of integer (int) and pointer (pointer to int), arg #1 [154] */
	sink_int(ptr);
}
