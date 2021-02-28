/*	$NetBSD: msg_229.c,v 1.3 2021/02/28 00:52:16 rillig Exp $	*/
# 3 "msg_229.c"

// Test for message: questionable conversion of function pointer [229]

typedef double (*unary_operator)(double);

int *
example(unary_operator op)
{
	return (int *)op;
}
