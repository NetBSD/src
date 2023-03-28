/*	$NetBSD: msg_263.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_263.c"

/* Test for message: \? undefined in traditional C [263] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \? undefined in traditional C [263] */
char ch = '\?';
