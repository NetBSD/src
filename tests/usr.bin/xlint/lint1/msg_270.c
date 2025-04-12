/*	$NetBSD: msg_270.c,v 1.6 2025/04/12 15:57:26 rillig Exp $	*/
# 3 "msg_270.c"

/* Test for message: function prototypes require C90 or later [270] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: function prototypes require C90 or later [270] */
void prototype(void);

char *traditional();

traditional_implicit_int();
