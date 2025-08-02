/*	$NetBSD: msg_264.c,v 1.5.2.1 2025/08/02 05:58:17 perseant Exp $	*/
# 3 "msg_264.c"

/* Test for message: \v requires C90 or later [264] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \v requires C90 or later [264] */
char ch = '\v';
/* expect+1: warning: \v requires C90 or later [264] */
char str[] = "vertical \v tab";
