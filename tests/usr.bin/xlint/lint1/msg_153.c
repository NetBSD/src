/*	$NetBSD: msg_153.c,v 1.6 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_153.c"

// Test for message: converting '%s' to incompatible '%s' for argument %d [153]


typedef double (*unary_operator)(double);

void sink_function_pointer(unary_operator);
void sink_int_pointer(int *);

void
to_function_pointer(int *x)
{
	/* expect+1: warning: converting 'pointer to int' to incompatible 'pointer to function(double) returning double' for argument 1 [153] */
	sink_function_pointer(x);
}

void
to_int_pointer(unary_operator op)
{
	/* expect+1: warning: converting 'pointer to function(double) returning double' to incompatible 'pointer to int' for argument 1 [153] */
	sink_int_pointer(op);
}
