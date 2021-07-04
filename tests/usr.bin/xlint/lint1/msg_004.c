/*	$NetBSD: msg_004.c,v 1.5 2021/07/04 13:32:35 rillig Exp $	*/
# 3 "msg_004.c"

// Test for message: illegal type combination [4]

// Lint does not detect "two or more data types", but GCC does.
signed double signed_double;

int ok_int;
double ok_double;
float _Complex ok_float_complex;

int _Complex illegal_int_complex;	/* expect: 4 *//* expect: 308 */

char enum {
	CHAR
};					/* expect: 4 */

long struct {
	int member;
};					/* expect: 4 */
