/*	$NetBSD: msg_246.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_246.c"

// Test for message: dubious conversion of enum to '%s' [246]
// This message is not used.

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
