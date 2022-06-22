/*	$NetBSD: msg_113.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_113.c"

// Test for message: cannot take address of register '%s' [113]

/* ARGSUSED */
void
example(register int arg)
{
	/* expect+1: error: cannot take address of register 'arg' [113] */
	return &arg;
}
