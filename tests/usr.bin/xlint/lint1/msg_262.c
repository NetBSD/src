/*	$NetBSD: msg_262.c,v 1.3 2021/06/29 07:17:43 rillig Exp $	*/
# 3 "msg_262.c"

/* Test for message: \" inside character constants undefined in traditional C [262] */

/* lint1-flags: -tw */

char msg = '\"';		/* expect: [262] */
