/*	$NetBSD: msg_263.c,v 1.6 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_263.c"

/* Test for message: \? undefined in traditional C [263] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \? undefined in traditional C [263] */
char ch = '\?';
/* expect+1: warning: \? undefined in traditional C [263] */
char str[] = "Hello\?";
