/*	$NetBSD: msg_301.c,v 1.3 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_301.c"

// Test for message: array of incomplete type [301]
// This message is not used.
// TODO: This message occurs in the code but is deactivated.

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;

/* expect+1: error: 'var' has incomplete type 'incomplete struct incomplete' [31] */
struct incomplete var[3];

/* expect+1: error: cannot take size/alignment of incomplete type [143] */
unsigned long sizeof_3 = sizeof(struct incomplete[3]);
