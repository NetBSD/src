/*	$NetBSD: msg_001.c,v 1.4 2021/01/31 11:23:01 rillig Exp $	*/
# 3 "msg_001.c"

// Test for message: old style declaration; add 'int' [1]

old_style = 1;			/* expect: [1] */

int new_style = 1;
