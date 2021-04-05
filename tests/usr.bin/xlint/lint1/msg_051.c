/*	$NetBSD: msg_051.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_051.c"

// Test for message: parameter mismatch: %d declared, %d defined [51]

void
example(int, int);

void
example(a, b, c)		/* expect: 231 *//* expect: 231 *//* expect: 231 */
    int a, b, c;
{				/* expect: 51 */
}
