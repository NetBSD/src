/*	$NetBSD: d_gcc_func.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "d_gcc_func.c"

/* gcc __FUNCTION__ */

/* lint1-extra-flags: -X 351 */

void
foo(const char *p)
{
	p = __FUNCTION__;
	foo(p);
}
