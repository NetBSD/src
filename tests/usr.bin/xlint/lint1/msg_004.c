/*	$NetBSD: msg_004.c,v 1.6 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_004.c"

// Test for message: illegal type combination [4]

// Lint does not detect "two or more data types", but GCC does.
signed double signed_double;

int ok_int;
double ok_double;
float _Complex ok_float_complex;

/* expect+2: error: invalid type for _Complex [308] */
/* expect+1: error: illegal type combination [4] */
int _Complex illegal_int_complex;

char enum {
	CHAR
};
/* expect-1: error: illegal type combination [4] */

long struct {
	int member;
};
/* expect-1: error: illegal type combination [4] */
