/*	$NetBSD: c90.c,v 1.3.2.1 2025/08/02 05:58:14 perseant Exp $	*/
# 3 "c90.c"

/*
 * Tests for the option -s, which allows features from C90, but neither any
 * later C standards nor GNU extensions.
 */

/* lint1-flags: -sw -X 351 */

/* expect+1: error: C90 to C17 require formal parameter before '...' [84] */
void varargs_function(...);

int
compound_literal(void)
{
	/* expect+1: error: compound literals are a C99/GCC extension [319] */
	return (int){123};
}
