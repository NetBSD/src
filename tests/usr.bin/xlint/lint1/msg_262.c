/*	$NetBSD: msg_262.c,v 1.4 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_262.c"

/* Test for message: \" inside character constants undefined in traditional C [262] */

/* lint1-flags: -tw */

/* expect+1: warning: \" inside character constants undefined in traditional C [262] */
char msg = '\"';
