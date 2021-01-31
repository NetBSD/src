/*	$NetBSD: d_c99_complex_num.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_complex_num.c"

double cabs(double _Complex);

double cabs(double _Complex foo)
{
	double d = __real__ foo;
	return d + 0.1fi;
}
