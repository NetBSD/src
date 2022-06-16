/*	$NetBSD: msg_238.c,v 1.5 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_238.c"

/* Test for message: initialization of union is illegal in traditional C [238] */

/* lint1-flags: -tw */

struct {
	int x;
} s = {
	3
};

union {
	int x;
/* expect+1: warning: initialization of union is illegal in traditional C [238] */
} u = {
	3
};
