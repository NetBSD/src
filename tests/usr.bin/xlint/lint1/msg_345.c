/*	$NetBSD: msg_345.c,v 1.2 2021/06/27 20:47:13 rillig Exp $	*/
# 3 "msg_345.c"

// Test for message: generic selection requires C11 or later [345]

/* Omit flag -g since it silences c11ism. */
/* lint1-flags: -Sw */

int
test(int x)
{
	/* expect+1: generic selection requires C11 or later [345] */
	return _Generic(x, default: 3) + x;
}
