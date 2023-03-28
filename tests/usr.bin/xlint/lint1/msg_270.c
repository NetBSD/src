/*	$NetBSD: msg_270.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_270.c"

/* Test for message: function prototypes are illegal in traditional C [270] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: function prototypes are illegal in traditional C [270] */
void prototype(void);

char *traditional();

traditional_implicit_int();
