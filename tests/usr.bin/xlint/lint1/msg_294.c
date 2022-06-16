/*	$NetBSD: msg_294.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_294.c"

// Test for message: multi-character character constant [294]

/* expect+2: warning: multi-character character constant [294] */
/* expect+1: warning: initializer does not fit [178] */
char ch = '1234';
