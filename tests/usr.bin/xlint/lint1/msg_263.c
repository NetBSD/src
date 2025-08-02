/*	$NetBSD: msg_263.c,v 1.6.2.1 2025/08/02 05:58:17 perseant Exp $	*/
# 3 "msg_263.c"

/* Test for message: \? requires C90 or later [263] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \? requires C90 or later [263] */
char ch = '\?';
/* expect+1: warning: \? requires C90 or later [263] */
char str[] = "Hello\?";
