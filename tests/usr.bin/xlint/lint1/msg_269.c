/*	$NetBSD: msg_269.c,v 1.6 2023/07/09 11:18:55 rillig Exp $	*/
# 3 "msg_269.c"

// Test for message: parameter '%s' declared inline [269]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: parameter 'x' declared inline [269] */
void example(inline int x);
