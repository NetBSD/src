/*	$NetBSD: msg_076.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_076.c"

// Test for message: character escape does not fit in character [76]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: character escape does not fit in character [76] */
char ch = '\777';
