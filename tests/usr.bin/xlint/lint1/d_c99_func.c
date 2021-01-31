/*	$NetBSD: d_c99_func.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_c99_func.c"

/* C99 __func__ */

void
foo(const char *p)
{
	p = __func__;
	foo(p);
}
