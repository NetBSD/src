/*	$NetBSD: msg_234.c,v 1.3 2022/06/11 11:20:40 rillig Exp $	*/
# 3 "msg_234.c"

// Test for message: union %s never defined [234]

/* expect+1: warning: union declared_but_not_defined never defined [234] */
union declared_but_not_defined;
