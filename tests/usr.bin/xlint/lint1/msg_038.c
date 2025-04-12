/*	$NetBSD: msg_038.c,v 1.5 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_038.c"

// Test for message: function invalid in structure or union [38]

/* lint1-extra-flags: -X 351 */

typedef void (function)(void);

struct {
	/* expect+1: error: function invalid in structure or union [38] */
	function fn;
} s;
