/*	$NetBSD: d_gcc_extension.c,v 1.5 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "d_gcc_extension.c"

/* extension */
void
a(void)
{
	double __logbw = 1;
	if (__extension__(({
		__typeof((__logbw)) x_ = (__logbw);
		!__builtin_isinf((x_)) && !__builtin_isnan((x_)); /* expect: 259 *//* expect: 259 */
	})))
		__logbw = 1;
}
