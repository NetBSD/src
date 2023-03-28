/*	$NetBSD: msg_291.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_291.c"

// Test for message: invalid multibyte character [291]

/* lint1-extra-flags: -X 351 */

char foreign[] = "\x80\xC3\x76";

/* expect+1: error: syntax error ':' [249] */
TODO: "Add example code that triggers the above message."
TODO: "Add example code that almost triggers the above message."
