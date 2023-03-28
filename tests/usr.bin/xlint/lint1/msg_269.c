/*	$NetBSD: msg_269.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_269.c"

// Test for message: argument '%s' declared inline [269]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: argument 'x' declared inline [269] */
void example(inline int x);
