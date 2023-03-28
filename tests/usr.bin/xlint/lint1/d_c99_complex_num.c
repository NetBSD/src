/*	$NetBSD: d_c99_complex_num.c,v 1.3 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_complex_num.c"

/* lint1-extra-flags: -X 351 */

double cabs(double _Complex);

double cabs(double _Complex foo)
{
	double d = __real__ foo;
	return d + 0.1fi;
}
