/*	$NetBSD: d_gcc_extension.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_gcc_extension.c"

/* extension */
void
a(void)
{
	double __logbw = 1;
	if (__extension__(({
		__typeof((__logbw)) x_ = (__logbw);
		!__builtin_isinf((x_)) && !__builtin_isnan((x_));
	})))
		__logbw = 1;
}
