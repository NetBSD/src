/*	$NetBSD: msg_291.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_291.c"

// Test for message: invalid multibyte character [291]

char foreign[] = "\x80\xC3\x76";

/* expect+1: error: syntax error ':' [249] */
TODO: "Add example code that triggers the above message."
TODO: "Add example code that almost triggers the above message."
