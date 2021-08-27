/*	$NetBSD: msg_235.c,v 1.3 2021/08/27 20:49:25 rillig Exp $	*/
# 3 "msg_235.c"

// Test for message: enum %s never defined [235]

/* expect+1: warning: enum declared_but_not_defined never defined [235] */
enum declared_but_not_defined;
