/*	$NetBSD: msg_266.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_266.c"

/* Test for message: 'long double' is illegal in traditional C [266] */

/* lint1-flags: -tw */

/* expect+1: warning: 'long double' is illegal in traditional C [266] */
long double ldbl = 0.0;
