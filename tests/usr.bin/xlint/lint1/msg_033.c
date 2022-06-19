/*	$NetBSD: msg_033.c,v 1.4 2022/06/19 12:14:34 rillig Exp $	*/
# 3 "msg_033.c"

// Test for message: duplicate member name '%s' [33]

/* lint1-extra-flags: -r */

struct {
	/* Despite the option '-r', this location is not mentioned. */
	int member;
	/* expect+1: error: duplicate member name 'member' [33] */
	double member;
};
