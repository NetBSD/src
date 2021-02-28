/*	$NetBSD: msg_153.c,v 1.5 2021/02/28 01:20:54 rillig Exp $	*/
# 3 "msg_153.c"

// Test for message: converting '%s' to incompatible '%s' for argument %d [153]


typedef double (*unary_operator)(double);

void sink_function_pointer(unary_operator);
void sink_int_pointer(int *);

void
to_function_pointer(int *x)
{
	sink_function_pointer(x);	/* expect: 153 */
}

void
to_int_pointer(unary_operator op)
{
	sink_int_pointer(op);		/* expect: 153 */
}
