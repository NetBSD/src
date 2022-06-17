/*	$NetBSD: msg_322.c,v 1.3 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_322.c"

/* Test for message: zero sized array is a C99 extension [322] */

/* lint1-flags: -sw */

/* expect+1: error: zero sized array is a C99 extension [322] */
typedef int empty_array[0];
