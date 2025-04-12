/*	$NetBSD: msg_262.c,v 1.6 2025/04/12 15:57:26 rillig Exp $	*/
# 3 "msg_262.c"

/* Test for message: \" inside a character constant requires C90 or later [262] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \" inside a character constant requires C90 or later [262] */
char msg = '\"';
