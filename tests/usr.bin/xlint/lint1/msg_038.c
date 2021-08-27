/*	$NetBSD: msg_038.c,v 1.3 2021/08/27 20:16:50 rillig Exp $	*/
# 3 "msg_038.c"

// Test for message: function illegal in structure or union [38]

typedef void (function)(void);

struct {
	/* expect+1: error: function illegal in structure or union [38] */
	function fn;
} s;
