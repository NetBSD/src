/*	$NetBSD: msg_322.c,v 1.4 2023/09/14 21:53:02 rillig Exp $	*/
# 3 "msg_322.c"

/* Test for message: zero sized array requires C99 or later [322] */

/* lint1-flags: -sw */

/* expect+1: error: zero sized array requires C99 or later [322] */
typedef int empty_array[0];
