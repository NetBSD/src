/*	$NetBSD: msg_000_c90.c,v 1.1 2021/07/08 05:18:49 rillig Exp $	*/
# 3 "msg_000_c90.c"

/*
 * Test for message: empty declaration [0]
 *
 * In strict C90 mode, an empty declaration is an error, not merely a warning.
 */

/* lint1-flags: -s */

/* expect+1: error: empty declaration [0] */
;
