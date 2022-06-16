/*	$NetBSD: msg_112.c,v 1.3 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_112.c"

// Test for message: cannot take address of bit-field [112]

struct {
	int bit_field:1;
} s;

/* expect+1: error: cannot take address of bit-field [112] */
void *ptr = &s.bit_field;
