/*	$NetBSD: msg_326.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_326.c"

// Test for message: attribute '%s' ignored for '%s' [326]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: attribute 'packed' ignored for 'int' [326] */
int variable __packed;
