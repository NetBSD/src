/*	$NetBSD: msg_153.c,v 1.8 2024/11/23 00:01:48 rillig Exp $	*/
# 3 "msg_153.c"

// Test for message: converting '%s' to incompatible '%s' for argument %d [153]

/* lint1-extra-flags: -X 351 */


typedef double (*unary_operator)(double);

void sink_function_pointer(unary_operator);
void sink_int_pointer(int *);
void sink_qualifiers(char *, const char *, volatile char *, const volatile char *);

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

void
qualifiers(char *ptr, const volatile char *cvptr)
{
	sink_qualifiers(ptr, ptr, ptr, ptr);

	/* expect+3: warning: converting 'pointer to const volatile char' to incompatible 'pointer to char' for argument 1 [153] */
	/* expect+2: warning: converting 'pointer to const volatile char' to incompatible 'pointer to const char' for argument 2 [153] */
	/* expect+1: warning: converting 'pointer to const volatile char' to incompatible 'pointer to volatile char' for argument 3 [153] */
	sink_qualifiers(cvptr, cvptr, cvptr, cvptr);
}
