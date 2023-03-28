/*	$NetBSD: msg_294.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_294.c"

// Test for message: multi-character character constant [294]

/* lint1-extra-flags: -X 351 */

/* expect+2: warning: multi-character character constant [294] */
/* expect+1: warning: initializer does not fit [178] */
char ch = '1234';
