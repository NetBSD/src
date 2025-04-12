/*	$NetBSD: msg_266.c,v 1.5 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_266.c"

/* Test for message: 'long double' is invalid in traditional C [266] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: 'long double' is invalid in traditional C [266] */
long double ldbl = 0.0;
