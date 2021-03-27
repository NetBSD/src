/*	$NetBSD: d_gcc_compound_statements1.c,v 1.4 2021/03/27 13:59:18 rillig Exp $	*/
# 3 "d_gcc_compound_statements1.c"

/* GCC compound statement with expression */

foo(unsigned long z)
{
	z = ({
		unsigned long tmp;
		tmp = 1;
		tmp;
	});
	foo(z);
}
