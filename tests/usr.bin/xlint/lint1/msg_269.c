/*	$NetBSD: msg_269.c,v 1.3 2021/08/22 13:45:56 rillig Exp $	*/
# 3 "msg_269.c"

// Test for message: argument declared inline: %s [269]

/* expect+1: warning: argument declared inline: x [269] */
void example(inline int x);
