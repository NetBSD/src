/*	$NetBSD: msg_308.c,v 1.4 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_308.c"

// Test for message: Invalid type for _Complex [308]

float _Complex float_complex;
double _Complex double_complex;
long double _Complex long_double_complex;
_Complex plain_complex;			/* expect: 308 */
int _Complex int_complex;		/* expect: 308, 4 */
