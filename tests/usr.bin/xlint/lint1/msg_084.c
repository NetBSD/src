/*	$NetBSD: msg_084.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_084.c"

// Test for message: ANSI C requires formal parameter before '...' [84]

void only_ellipsis(...)
{
}

void ok_ellipsis(const char *fmt, ...)
{
}
