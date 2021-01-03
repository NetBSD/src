/*	$NetBSD: msg_031.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_031.c"

// Test for message: incomplete structure or union %s: %s [31]

struct complete {
	int dummy;
};

struct incomplete;


struct complete complete_var;

struct incomplete incomplete_var;
