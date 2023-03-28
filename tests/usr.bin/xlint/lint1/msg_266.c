/*	$NetBSD: msg_266.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_266.c"

/* Test for message: 'long double' is illegal in traditional C [266] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: 'long double' is illegal in traditional C [266] */
long double ldbl = 0.0;
