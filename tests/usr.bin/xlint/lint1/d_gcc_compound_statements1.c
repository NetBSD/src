/*	$NetBSD: d_gcc_compound_statements1.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_gcc_compound_statements1.c"

/* GCC compound statements */

foo(unsigned long z)
{
	z = ({
		unsigned long tmp;
		tmp = 1;
		tmp;
	});
	foo(z);
}
