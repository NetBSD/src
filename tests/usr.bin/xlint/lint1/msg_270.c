/*	$NetBSD: msg_270.c,v 1.5 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_270.c"

/* Test for message: function prototypes are invalid in traditional C [270] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: function prototypes are invalid in traditional C [270] */
void prototype(void);

char *traditional();

traditional_implicit_int();
