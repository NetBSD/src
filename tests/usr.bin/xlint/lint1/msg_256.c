/*	$NetBSD: msg_256.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_256.c"

// Test for message: unterminated comment [256]

int dummy;

/* expect+2: error: unterminated comment [256] */
/* This comment never ends.
