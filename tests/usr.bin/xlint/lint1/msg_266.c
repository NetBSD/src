/*	$NetBSD: msg_266.c,v 1.4.2.1 2025/08/02 05:58:17 perseant Exp $	*/
# 3 "msg_266.c"

/* Test for message: 'long double' requires C90 or later [266] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: 'long double' requires C90 or later [266] */
long double ldbl = 0.0;
