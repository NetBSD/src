/*	$NetBSD: msg_038.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_038.c"

// Test for message: function illegal in structure or union [38]

/* lint1-extra-flags: -X 351 */

typedef void (function)(void);

struct {
	/* expect+1: error: function illegal in structure or union [38] */
	function fn;
} s;
