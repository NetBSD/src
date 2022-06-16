/*	$NetBSD: msg_263.c,v 1.4 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_263.c"

/* Test for message: \? undefined in traditional C [263] */

/* lint1-flags: -tw */

/* expect+1: warning: \? undefined in traditional C [263] */
char ch = '\?';
