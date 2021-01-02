/*	$NetBSD: msg_004.c,v 1.2 2021/01/02 18:06:01 rillig Exp $	*/
# 3 "msg_004.c"

// Test for message: illegal type combination [4]

// XXX: this goes undetected
signed double signed_double;

int ok_int;
double ok_double;
float _Complex ok_float_complex;

int _Complex illegal_int_complex;

char enum {
	CHAR
};

long struct {
	int member;
};
