/*	$NetBSD: msg_264.c,v 1.5 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_264.c"

/* Test for message: \v undefined in traditional C [264] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \v undefined in traditional C [264] */
char ch = '\v';
/* expect+1: warning: \v undefined in traditional C [264] */
char str[] = "vertical \v tab";
