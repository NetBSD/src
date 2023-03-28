/*	$NetBSD: msg_153.c,v 1.7 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_153.c"

// Test for message: converting '%s' to incompatible '%s' for argument %d [153]

/* lint1-extra-flags: -X 351 */


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
