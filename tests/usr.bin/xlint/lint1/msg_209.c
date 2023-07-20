/*	$NetBSD: msg_209.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_209.c"

// Test for message: continue outside loop [209]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	/* expect+1: error: continue outside loop [209] */
	continue;
}
