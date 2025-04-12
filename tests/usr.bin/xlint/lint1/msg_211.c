/*	$NetBSD: msg_211.c,v 1.9 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_211.c"

// Test for message: function has return type '%s' but returns '%s' [211]

/* lint1-extra-flags: -X 351 */

struct str {
	int member;
};

int
return_int(double dbl, void *ptr, struct str str)
{
	if (dbl > 0.0)
		return dbl;
	if (ptr != (void *)0)
		/* expect+1: warning: invalid combination of integer 'int' and pointer 'pointer to void' for 'return' [183] */
		return ptr;
	if (str.member > 0)
		/* expect+1: error: function has return type 'int' but returns 'struct str' [211] */
		return str;
	return 3;
}

enum A {
	A
};

enum B {
	B
};

enum A
return_enum(enum B arg)
{
	/* expect+1: warning: function has return type 'enum A' but returns 'enum B' [211] */
	return arg;
}
