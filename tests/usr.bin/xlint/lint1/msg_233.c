/*	$NetBSD: msg_233.c,v 1.3 2021/08/27 20:49:25 rillig Exp $	*/
# 3 "msg_233.c"

// Test for message: struct %s never defined [233]

/* expect+1: warning: struct declared_but_not_defined never defined [233] */
struct declared_but_not_defined;
