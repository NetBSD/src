/*	$NetBSD: msg_308.c,v 1.8.2.1 2025/08/02 05:58:18 perseant Exp $	*/
# 3 "msg_308.c"

// Test for message: invalid type for _Complex [308]

/* lint1-extra-flags: -X 351 */

float _Complex float_complex;
double _Complex double_complex;
long double _Complex long_double_complex;

/* expect+1: error: invalid type for _Complex [308] */
_Complex plain_complex;

/* expect+2: error: invalid type for _Complex [308] */
/* expect+1: error: invalid type combination [4] */
int _Complex int_complex;
