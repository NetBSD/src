/*	$NetBSD: msg_270.c,v 1.4.2.1 2025/08/02 05:58:17 perseant Exp $	*/
# 3 "msg_270.c"

/* Test for message: function prototypes require C90 or later [270] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: function prototypes require C90 or later [270] */
void prototype(void);

char *traditional();

traditional_implicit_int();
