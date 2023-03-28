/*	$NetBSD: msg_251.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_251.c"

// Test for message: malformed integer constant [251]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: malformed integer constant [251] */
int lll = 123LLL;

/* expect+1: warning: malformed integer constant [251] */
unsigned int uu = 123UU;
