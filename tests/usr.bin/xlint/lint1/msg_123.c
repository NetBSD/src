/*	$NetBSD: msg_123.c,v 1.6 2022/06/19 12:14:34 rillig Exp $	*/
# 3 "msg_123.c"

// Test for message: illegal combination of %s '%s' and %s '%s', op '%s' [123]

void ok(_Bool);
void bad(_Bool);

void
compare(_Bool b, int i, double d, const char *p)
{
	ok(b < b);
	ok(b < i);
	ok(b < d);
	/* expect+1: warning: illegal combination of integer '_Bool' and pointer 'pointer to const char', op '<' [123] */
	bad(b < p);
	ok(i < b);
	ok(i < i);
	ok(i < d);
	/* expect+1: warning: illegal combination of integer 'int' and pointer 'pointer to const char', op '<' [123] */
	bad(i < p);
	ok(d < b);
	ok(d < i);
	ok(d < d);
	/* expect+1: error: operands of '<' have incompatible types 'double' and 'pointer' [107] */
	bad(d < p);
	/* expect+1: warning: illegal combination of pointer 'pointer to const char' and integer '_Bool', op '<' [123] */
	bad(p < b);
	/* expect+1: warning: illegal combination of pointer 'pointer to const char' and integer 'int', op '<' [123] */
	bad(p < i);
	/* expect+1: error: operands of '<' have incompatible types 'pointer' and 'double' [107] */
	bad(p < d);
	ok(p < p);
}

void
cover_check_assign_types_compatible(int *int_pointer, int i)
{
	/* expect+1: warning: illegal combination of pointer 'pointer to int' and integer 'int', op '=' [123] */
	int_pointer = i;
	/* expect+1: warning: illegal combination of integer 'int' and pointer 'pointer to int', op '=' [123] */
	i = int_pointer;
}
