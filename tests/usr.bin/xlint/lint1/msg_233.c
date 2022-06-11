/*	$NetBSD: msg_233.c,v 1.4 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "msg_233.c"

// Test for message: struct '%s' never defined [233]

/* expect+1: warning: struct 'declared_but_not_defined' never defined [233] */
struct declared_but_not_defined;
