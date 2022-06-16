/*	$NetBSD: msg_229.c,v 1.6 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_229.c"

// Test for message: converting '%s' to '%s' is questionable [229]

typedef double (*unary_operator)(double);

int *
to_int_pointer(unary_operator op)
{
	/* expect+1: warning: converting 'pointer to function(double) returning double' to 'pointer to int' is questionable [229] */
	return (int *)op;
}

unary_operator
to_function_pointer(int *p)
{
	/* expect+1: warning: converting 'pointer to int' to 'pointer to function(double) returning double' is questionable [229] */
	return (unary_operator)p;
}
