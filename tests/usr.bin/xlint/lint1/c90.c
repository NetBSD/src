/*	$NetBSD: c90.c,v 1.3 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "c90.c"

/*
 * Tests for the option -s, which allows features from C90, but neither any
 * later C standards nor GNU extensions.
 */

/* lint1-flags: -sw -X 351 */

/* expect+1: error: C90 to C17 require formal parameter before '...' [84] */
void varargs_function(...);
