/*	$NetBSD: msg_272_c90.c,v 1.1 2021/07/08 05:18:49 rillig Exp $	*/
# 3 "msg_272_c90.c"

/*
 * Test for message: empty translation unit [272]
 *
 * In strict C90 mode, an empty translation unit is an error, not merely a
 * warning.
 */

/* lint1-flags: -s */

/* expect+1: error: empty translation unit [272] */
