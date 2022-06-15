/*	$NetBSD: msg_051.c,v 1.5 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_051.c"

// Test for message: parameter mismatch: %d declared, %d defined [51]

void
example(int, int);

void
/* expect+3: warning: argument 'a' unused in function 'example' [231] */
/* expect+2: warning: argument 'b' unused in function 'example' [231] */
/* expect+1: warning: argument 'c' unused in function 'example' [231] */
example(a, b, c)
    int a, b, c;
/* expect+1: error: parameter mismatch: 2 declared, 3 defined [51] */
{
}
