/*	$NetBSD: msg_153.c,v 1.3 2021/02/28 00:52:16 rillig Exp $	*/
# 3 "msg_153.c"

// Test for message: argument has incompatible pointer type, arg #%d (%s != %s) [153]


typedef double (*unary_operator)(double);

void sink_unary_operator(unary_operator);

void
example(int x)
{
	sink_unary_operator(&x);
}
