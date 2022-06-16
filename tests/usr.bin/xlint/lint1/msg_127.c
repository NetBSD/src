/*	$NetBSD: msg_127.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_127.c"

/* Test for message: '&' before array or function: ignored [127] */

/* lint1-extra-flags: -t */

void
example()
{
	/* expect+1: warning: '&' before array or function: ignored [127] */
	if (&example != (void *)0)
		return;
}
