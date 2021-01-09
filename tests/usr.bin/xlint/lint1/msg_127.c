/*	$NetBSD: msg_127.c,v 1.2 2021/01/09 14:37:16 rillig Exp $	*/
# 3 "msg_127.c"

/* Test for message: '&' before array or function: ignored [127] */

/* lint1-extra-flags: -t */

void
example(void)
{
	if (&example != (void *)0)
		return;
}
