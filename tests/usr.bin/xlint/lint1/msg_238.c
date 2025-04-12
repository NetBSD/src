/*	$NetBSD: msg_238.c,v 1.7 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_238.c"

/* Test for message: initialization of union is invalid in traditional C [238] */

/* lint1-flags: -tw -X 351 */

struct {
	int x;
} s = {
	3
};

union {
	int x;
/* expect+1: warning: initialization of union is invalid in traditional C [238] */
} u = {
	3
};
