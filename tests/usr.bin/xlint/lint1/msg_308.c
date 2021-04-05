/*	$NetBSD: msg_308.c,v 1.6 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_308.c"

// Test for message: invalid type for _Complex [308]

float _Complex float_complex;
double _Complex double_complex;
long double _Complex long_double_complex;
_Complex plain_complex;			/* expect: 308 */
int _Complex int_complex;		/* expect: 308 *//* expect: 4 */
