/*	$NetBSD: msg_185.c,v 1.6 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_185.c"

// Test for message: cannot initialize '%s' from '%s' [185]

typedef struct any {
	const void *value;
} any;

void use(const void *);

void
initialization_with_redundant_braces(any arg)
{
	/* expect+1: error: cannot initialize 'pointer to const void' from 'double' [185] */
	any local = { 3.0 };
	use(&arg);
}
