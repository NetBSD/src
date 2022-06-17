/*	$NetBSD: msg_345.c,v 1.3 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_345.c"

// Test for message: generic selection requires C11 or later [345]

/* Omit flag -g since it silences c11ism. */
/* lint1-flags: -Sw */

int
test(int x)
{
	/* expect+1: error: generic selection requires C11 or later [345] */
	return _Generic(x, default: 3) + x;
}
