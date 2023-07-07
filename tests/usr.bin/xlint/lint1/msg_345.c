/*	$NetBSD: msg_345.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_345.c"

// Test for message: generic selection requires C11 or later [345]

/* Omit flag -g since it silences c11ism. */
/* lint1-flags: -Sw -X 351 */

int
test(int x)
{
	/* expect+1: error: generic selection requires C11 or later [345] */
	return _Generic(x, default: 3) + x;
}
