/*	$NetBSD: d_gcc_compound_statements1.c,v 1.5 2021/06/19 15:51:11 rillig Exp $	*/
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

/*
 * Compound statements are only allowed in functions, not at file scope.
 *
 * Before decl.c 1.186 from 2021-06-19, lint crashed with a segmentation
 * fault.
 */
int c = ({
    return 3;		/* expect: return outside function */
});			/* expect: cannot initialize 'int' from 'void' */
