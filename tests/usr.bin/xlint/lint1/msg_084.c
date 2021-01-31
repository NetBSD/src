/*	$NetBSD: msg_084.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_084.c"

// Test for message: ANSI C requires formal parameter before '...' [84]

void only_ellipsis(...)		/* expect: 84 */
{
}

void ok_ellipsis(const char *fmt, ...)	/* expect: 231 */
{
}
