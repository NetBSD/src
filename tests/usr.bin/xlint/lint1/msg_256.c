/*	$NetBSD: msg_256.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_256.c"

// Test for message: unterminated comment [256]

typedef int dummy;

/* expect+2: error: unterminated comment [256] */
/* This comment never ends.
