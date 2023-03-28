/*	$NetBSD: msg_262.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_262.c"

/* Test for message: \" inside character constants undefined in traditional C [262] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \" inside character constants undefined in traditional C [262] */
char msg = '\"';
