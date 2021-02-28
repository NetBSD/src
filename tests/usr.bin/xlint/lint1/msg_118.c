/*	$NetBSD: msg_118.c,v 1.3 2021/02/28 01:22:02 rillig Exp $	*/
# 3 "msg_118.c"

// Test for message: semantics of '%s' change in ANSI C; use explicit cast [118]

/* lint1-extra-flags: -h */

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
