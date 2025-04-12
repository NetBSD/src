/*	$NetBSD: msg_263.c,v 1.7 2025/04/12 15:57:26 rillig Exp $	*/
# 3 "msg_263.c"

/* Test for message: \? requires C90 or later [263] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \? requires C90 or later [263] */
char ch = '\?';
/* expect+1: warning: \? requires C90 or later [263] */
char str[] = "Hello\?";
