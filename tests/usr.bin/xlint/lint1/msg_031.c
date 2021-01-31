/*	$NetBSD: msg_031.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_031.c"

// Test for message: incomplete structure or union %s: %s [31]

struct complete {
	int dummy;
};

struct incomplete;			/* expect: 233 */


struct complete complete_var;

struct incomplete incomplete_var;	/* expect: 31 */
