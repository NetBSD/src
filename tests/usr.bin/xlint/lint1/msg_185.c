/*	$NetBSD: msg_185.c,v 1.4 2021/03/18 21:20:21 rillig Exp $	*/
# 3 "msg_185.c"

// Test for message: initialization type mismatch (%s) and (%s) [185]

typedef struct any {
	const void *value;
} any;

void use(const void *);

void
initialization_with_redundant_braces(any arg)
{
	any local = { 3.0 };	/* expect: 185 */
	use(&arg);
}

// TODO: message 185 needs to be reworded to "cannot initialize '%s' from '%s'".
