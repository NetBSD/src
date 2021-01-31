/*	$NetBSD: d_gcc_func.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_gcc_func.c"

/* gcc __FUNCTION__ */

void
foo(const char *p)
{
	p = __FUNCTION__;
	foo(p);
}
