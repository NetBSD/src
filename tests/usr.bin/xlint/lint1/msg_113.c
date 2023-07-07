/*	$NetBSD: msg_113.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_113.c"

// Test for message: cannot take address of register '%s' [113]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
void
example(register int arg)
{
	/* expect+1: error: cannot take address of register 'arg' [113] */
	return &arg;
}
