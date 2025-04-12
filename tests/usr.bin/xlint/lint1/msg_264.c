/*	$NetBSD: msg_264.c,v 1.6 2025/04/12 15:57:26 rillig Exp $	*/
# 3 "msg_264.c"

/* Test for message: \v requires C90 or later [264] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \v requires C90 or later [264] */
char ch = '\v';
/* expect+1: warning: \v requires C90 or later [264] */
char str[] = "vertical \v tab";
