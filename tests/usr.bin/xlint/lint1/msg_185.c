/*	$NetBSD: msg_185.c,v 1.7 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_185.c"

// Test for message: cannot initialize '%s' from '%s' [185]

/* lint1-extra-flags: -X 351 */

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
