/*	$NetBSD: msg_112.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_112.c"

// Test for message: cannot take address of bit-field [112]

/* lint1-extra-flags: -X 351 */

struct {
	int bit_field:1;
} s;

/* expect+1: error: cannot take address of bit-field [112] */
void *ptr = &s.bit_field;
