/*	$NetBSD: expr_binary_trad.c,v 1.2 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "expr_binary_trad.c"

/*
 * Test binary operators in traditional C.
 */

/* lint1-flags: -tw */

struct incompatible {		/* just to generate the error message */
	int member;
};
struct incompatible sink;

/*
 * Test the usual arithmetic conversions.
 *
 * C99 6.3.1.8 "Usual arithmetic conversions"
 */
void
cover_balance()
{

	/* expect+1: ... 'pointer to char' ... */
	sink = (char *)0 + 0;

	/* expect+1: ... 'pointer to char' ... */
	sink = 0 + (char *)0;

	/* expect+1: ... 'int' ... */
	sink = 1 + 1;

	/* expect+1: ... 'double' ... */
	sink = 0.0 + 0;
	/* expect+1: ... 'double' ... */
	sink = 0 + 0.0;
	/* expect+1: ... 'double' ... */
	sink = 0.0 + (float)0.0;
	/* expect+1: ... 'double' ... */
	sink = (float)0.0 + 0.0;

	/*
	 * In traditional C, 'float' gets promoted to 'double' before
	 * applying the usual arithmetic conversions; see 'promote'.
	 */
	/* expect+1: ... 'double' ... */
	sink = (float)0.0 + 0;
	/* expect+1: ... 'double' ... */
	sink = 0 + (float)0.0;

	/* expect+1: ... 'unsigned long' ... */
	sink = (unsigned long)0 + 0;
	/* expect+1: ... 'unsigned long' ... */
	sink = 0 + (unsigned long)0;

	/* expect+1: ... 'unsigned long' ... */
	sink = (unsigned long)0 + (long)0;
	/* expect+1: ... 'unsigned long' ... */
	sink = (long)0 + (unsigned long)0;

	/*
	 * In traditional C, if one of the operands is unsigned, the result
	 * is unsigned as well.
	 */
	/* expect+1: ... 'unsigned long' ... */
	sink = (unsigned)0 + (long)0;
}
