/*	$NetBSD: msg_084.c,v 1.6 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "msg_084.c"

/* Test for message: C90 to C17 require formal parameter before '...' [84] */

/* lint1-flags: -sw -X 351 */

/* expect+2: error: C90 to C17 require formal parameter before '...' [84] */
void
only_ellipsis(...)
{
}

char
ok_ellipsis(const char *fmt, ...)
{
	return fmt[0];
}
