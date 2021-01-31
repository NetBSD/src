/*	$NetBSD: d_c99_func.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_func.c"

/* C99 __func__ */

void
foo(const char *p) {
	p = __func__;
	foo(p);
}
