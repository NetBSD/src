/*	$NetBSD: msg_269.c,v 1.4 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "msg_269.c"

// Test for message: argument '%s' declared inline [269]

/* expect+1: warning: argument 'x' declared inline [269] */
void example(inline int x);
