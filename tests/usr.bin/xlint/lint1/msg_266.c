/*	$NetBSD: msg_266.c,v 1.6 2025/04/12 15:57:26 rillig Exp $	*/
# 3 "msg_266.c"

/* Test for message: 'long double' requires C90 or later [266] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: 'long double' requires C90 or later [266] */
long double ldbl = 0.0;
