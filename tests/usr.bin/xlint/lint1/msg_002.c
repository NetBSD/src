/*	$NetBSD: msg_002.c,v 1.4 2021/07/08 05:18:49 rillig Exp $	*/
# 3 "msg_002.c"

// Test for message: empty declaration [2]

/* expect+1: warning: empty declaration [2] */
int;

int local_variable;

/* expect+1: warning: empty declaration [2] */
const;
