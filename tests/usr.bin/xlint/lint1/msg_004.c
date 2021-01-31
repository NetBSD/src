/*	$NetBSD: msg_004.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_004.c"

// Test for message: illegal type combination [4]

// XXX: this goes undetected
signed double signed_double;

int ok_int;
double ok_double;
float _Complex ok_float_complex;

int _Complex illegal_int_complex;	/* expect: 4, 308 */

char enum {
	CHAR
};					/* expect: 4 */

long struct {
	int member;
};					/* expect: 4 */
