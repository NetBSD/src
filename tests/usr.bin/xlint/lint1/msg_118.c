/*	$NetBSD: msg_118.c,v 1.4 2021/04/06 21:59:58 rillig Exp $	*/
# 3 "msg_118.c"

/* Test for message: semantics of '%s' change in ANSI C; use explicit cast [118] */

/* lint1-flags: -hsw */

int
int_shl_uint(int i, unsigned int u)
{
	return i << u;
}

unsigned
uint_shl_ulong(unsigned a, unsigned long ul)
{
	return a << ul;		/* expect: 118 */
}
