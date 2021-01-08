/*	$NetBSD: msg_051.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_051.c"

// Test for message: parameter mismatch: %d declared, %d defined [51]

void
example(int, int);

void
example(a, b, c)
    int a, b, c;
{
}
