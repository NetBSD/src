/*	$NetBSD: msg_246.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_246.c"

// Test for message: dubious conversion of enum to '%s' [246]
// This message is not used.

/* lint1-extra-flags: -X 351 */

enum color {
	RED, GREEN, BLUE
};

double
to_double(enum color c)
{
	return c;
}

void *
to_pointer(enum color c)
{
	/* expect+1: warning: illegal combination of pointer 'pointer to void' and integer 'enum color' [183] */
	return c;
}
