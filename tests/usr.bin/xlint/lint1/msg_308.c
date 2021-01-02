/*	$NetBSD: msg_308.c,v 1.2 2021/01/02 16:55:45 rillig Exp $	*/
# 3 "msg_308.c"

// Test for message: Invalid type %s for _Complex [308]

float _Complex float_complex;
double _Complex double_complex;
long double _Complex long_double_complex;
// FIXME: aborts: _Complex plain_complex;
// FIXME: aborts: int _Complex int_complex;

TODO: "Add example code that triggers the above message."
TODO: "Add example code that almost triggers the above message."
