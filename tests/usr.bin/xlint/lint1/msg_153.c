/*	$NetBSD: msg_153.c,v 1.9 2024/11/23 16:48:35 rillig Exp $	*/
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

	/* expect+3: warning: passing 'pointer to const volatile char' to argument 1 discards 'const volatile' [383] */
	/* expect+2: warning: passing 'pointer to const volatile char' to argument 2 discards 'volatile' [383] */
	/* expect+1: warning: passing 'pointer to const volatile char' to argument 3 discards 'const' [383] */
	sink_qualifiers(cvptr, cvptr, cvptr, cvptr);
}
