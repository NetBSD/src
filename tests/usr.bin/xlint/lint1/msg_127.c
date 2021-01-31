/*	$NetBSD: msg_127.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_127.c"

/* Test for message: '&' before array or function: ignored [127] */

/* lint1-extra-flags: -t */

void
example()
{
	if (&example != (void *)0)	/* expect: 127 */
		return;
}
