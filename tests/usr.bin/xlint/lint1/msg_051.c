/*	$NetBSD: msg_051.c,v 1.7 2023/07/09 11:18:55 rillig Exp $	*/
# 3 "msg_051.c"

// Test for message: parameter mismatch: %d declared, %d defined [51]

/* lint1-extra-flags: -X 351 */

void
example(int, int);

void
/* expect+3: warning: parameter 'a' unused in function 'example' [231] */
/* expect+2: warning: parameter 'b' unused in function 'example' [231] */
/* expect+1: warning: parameter 'c' unused in function 'example' [231] */
example(a, b, c)
    int a, b, c;
/* expect+1: error: parameter mismatch: 2 declared, 3 defined [51] */
{
}
