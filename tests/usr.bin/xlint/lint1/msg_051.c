/*	$NetBSD: msg_051.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_051.c"

// Test for message: parameter mismatch: %d declared, %d defined [51]

void
example(int, int);

void
example(a, b, c)		/* expect: 231, 231, 231 */
    int a, b, c;
{				/* expect: 51 */
}
