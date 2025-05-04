/*	$NetBSD: msg_153.c,v 1.12 2025/05/04 08:37:09 rillig Exp $	*/
# 3 "msg_153.c"

// Test for message: converting '%s' to incompatible '%s' for argument %d [153]

/* lint1-extra-flags: -X 351 */


typedef double (*unary_operator)(double);
typedef unsigned char sixteen_bytes[16];

void sink_function_pointer(unary_operator);
void sink_int_pointer(int *);
void sink_qualifiers(char *, const char *, volatile char *, const volatile char *);
void take_pointer_to_sixteen_bytes(sixteen_bytes *);

sixteen_bytes bytes;

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

	/* expect+3: warning: passing 'pointer to const volatile char' as argument 1 to 'sink_qualifiers' discards 'const volatile' [383] */
	/* expect+2: warning: passing 'pointer to const volatile char' as argument 2 to 'sink_qualifiers' discards 'volatile' [383] */
	/* expect+1: warning: passing 'pointer to const volatile char' as argument 3 to 'sink_qualifiers' discards 'const' [383] */
	sink_qualifiers(cvptr, cvptr, cvptr, cvptr);
}

void
pass_pointer_to_array(void)
{
	take_pointer_to_sixteen_bytes(&bytes);
}
