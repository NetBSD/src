/*	$NetBSD: msg_264.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_264.c"

/* Test for message: \v undefined in traditional C [264] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \v undefined in traditional C [264] */
char str[] = "vertical \v tab";
