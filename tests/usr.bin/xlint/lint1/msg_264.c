/*	$NetBSD: msg_264.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_264.c"

/* Test for message: \v undefined in traditional C [264] */

/* lint1-flags: -tw */

/* expect+1: warning: \v undefined in traditional C [264] */
char str[] = "vertical \v tab";
