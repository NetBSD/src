/*	$NetBSD: msg_229.c,v 1.5 2021/02/28 01:20:54 rillig Exp $	*/
# 3 "msg_229.c"

// Test for message: converting '%s' to '%s' is questionable [229]

typedef double (*unary_operator)(double);

int *
to_int_pointer(unary_operator op)
{
	return (int *)op;		/* expect: 229 */
}

unary_operator
to_function_pointer(int *p)
{
	return (unary_operator)p;	/* expect: 229 */
}
