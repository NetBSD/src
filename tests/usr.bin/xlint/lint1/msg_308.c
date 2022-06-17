/*	$NetBSD: msg_308.c,v 1.7 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_308.c"

// Test for message: invalid type for _Complex [308]

float _Complex float_complex;
double _Complex double_complex;
long double _Complex long_double_complex;

/* expect+1: error: invalid type for _Complex [308] */
_Complex plain_complex;

/* expect+2: error: invalid type for _Complex [308] */
/* expect+1: error: illegal type combination [4] */
int _Complex int_complex;
