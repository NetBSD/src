/*	$NetBSD: msg_238.c,v 1.4 2021/03/29 22:24:34 rillig Exp $	*/
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
} u = {				/* expect: 238 */
	3
};
