/*	$NetBSD: msg_001_c90.c,v 1.3 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_001_c90.c"

/*
 * Test for message: old-style declaration; add 'int' [1]
 *
 * In strict C90 mode, an old-style declaration is an error, not merely a
 * warning.
 */

/* lint1-flags: -s -X 351 */

/* expect+1: error: old-style declaration; add 'int' [1] */
implicit_global_variable;
