/*	$NetBSD: c90.c,v 1.2 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "c90.c"

/*
 * Tests for the option -s, which allows features from C90, but neither any
 * later C standards nor GNU extensions.
 */

/* lint1-flags: -sw -X 351 */

/* expect+1: error: ANSI C requires formal parameter before '...' [84] */
void varargs_function(...);
