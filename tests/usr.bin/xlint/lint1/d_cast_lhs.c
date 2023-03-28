/*	$NetBSD: d_cast_lhs.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_cast_lhs.c"

/*
 * Pointer casts had been valid lvalues in GCC before 4.0.
 *
 * https://gcc.gnu.org/onlinedocs/gcc-3.4.6/gcc/Lvalues.html#Lvalues
 *
 * C99 6.5.4 "Cast operators" footnote 85 says "A cast does not yield an
 * lvalue".
 */

/* lint1-extra-flags: -X 351 */

struct str {
	int member;
};

void sink(const void *);

/* ARGSUSED */
void
foo(void *p)
{
	/* expect+2: error: a cast does not yield an lvalue [163] */
	/* expect+1: error: left operand of '=' must be lvalue [114] */
	((struct str *)p) = 0;

	/* expect+2: error: a cast does not yield an lvalue [163] */
	/* expect+1: error: operand of '&' must be lvalue [114] */
	sink(&(const void *)p);
}
