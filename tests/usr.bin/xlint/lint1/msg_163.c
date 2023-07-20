/*	$NetBSD: msg_163.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_163.c"

// Test for message: a cast does not yield an lvalue [163]

/* lint1-extra-flags: -X 351 */

void
example(char *p, int i)
{
	p++;

	/*
	 * Using a cast as an lvalue had been a GCC extension until 3.4.
	 * It was removed in GCC 4.0.
	 *
	 * https://gcc.gnu.org/onlinedocs/gcc-3.4.6/gcc/Lvalues.html#Lvalues
	 * https://gcc.gnu.org/onlinedocs/gcc-4.0.4/gcc/index.html#toc_C-Extensions
	 */
	/* expect+2: error: a cast does not yield an lvalue [163] */
	/* expect+1: error: operand of 'x++' must be lvalue [114] */
	((char *)p)++;

	i++;

	/* expect+2: error: a cast does not yield an lvalue [163] */
	/* expect+1: error: operand of 'x++' must be lvalue [114] */
	((int)i)++;
}
