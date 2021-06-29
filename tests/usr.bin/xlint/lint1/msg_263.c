/*	$NetBSD: msg_263.c,v 1.3 2021/06/29 07:17:43 rillig Exp $	*/
# 3 "msg_263.c"

/* Test for message: \? undefined in traditional C [263] */

/* lint1-flags: -tw */

char ch = '\?';			/* expect: [263] */
