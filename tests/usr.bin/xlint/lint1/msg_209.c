/*	$NetBSD: msg_209.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_209.c"

// Test for message: continue outside loop [209]

void
example(void)
{
	/* expect+1: error: continue outside loop [209] */
	continue;
}
