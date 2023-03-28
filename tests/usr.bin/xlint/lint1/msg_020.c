/*	$NetBSD: msg_020.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_020.c"

// Test for message: negative array dimension (%d) [20]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: negative array dimension (-3) [20] */
int array[-3];
