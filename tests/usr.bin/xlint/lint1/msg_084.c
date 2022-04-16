/*	$NetBSD: msg_084.c,v 1.4 2022/04/16 09:20:01 rillig Exp $	*/
# 3 "msg_084.c"

/* Test for message: ANSI C requires formal parameter before '...' [84] */

/* lint1-flags: -sw */

/* expect+2: error: ANSI C requires formal parameter before '...' [84] */
void
only_ellipsis(...)
{
}

char
ok_ellipsis(const char *fmt, ...)
{
	return fmt[0];
}
