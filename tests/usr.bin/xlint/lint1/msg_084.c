/*	$NetBSD: msg_084.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_084.c"

/* Test for message: ANSI C requires formal parameter before '...' [84] */

/* lint1-flags: -sw -X 351 */

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
