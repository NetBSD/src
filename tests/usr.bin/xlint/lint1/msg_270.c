/*	$NetBSD: msg_270.c,v 1.3 2021/08/22 13:45:56 rillig Exp $	*/
# 3 "msg_270.c"

/* Test for message: function prototypes are illegal in traditional C [270] */

/* lint1-flags: -tw */

/* expect+1: warning: function prototypes are illegal in traditional C [270] */
void prototype(void);

char *traditional();

traditional_implicit_int();
