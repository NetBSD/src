/*	$NetBSD: msg_238.c,v 1.6 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_238.c"

/* Test for message: initialization of union is illegal in traditional C [238] */

/* lint1-flags: -tw -X 351 */

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
