/*	$NetBSD: expr_range.c,v 1.1 2021/05/14 21:14:55 rillig Exp $	*/
# 3 "expr_range.c"

/*
 * Test whether lint can detect switch branches that are impossible to reach.
 * As of 2021-05-14, it cannot.  To do this, it would need to keep track of
 * the possible values of each variable or expression.  To do this reliably,
 * it would also need accurate control flow analysis, which as of 2021-05-14
 * works only for functions without 'goto'.
 *
 * GCC 10 does not complain the unreachable branch.  It knows that the branch
 * is unreachable though since it doesn't generate any code for it.  GCC once
 * had the option -Wunreachable-code, but that option was made a no-op in
 * 2011.
 *
 * Clang 10 does not complain about this either, and just like GCC it doesn't
 * generate any code for this branch.  The code for tracking an expression's
 * possible values may be related to RangeConstraintManager, just guessing.
 *
 * There should be at least one static analysis tool that warns about this.
 */

/* lint1-extra-flags: -chap */

void
example(unsigned x)
{
	switch (x & 6) {
	case 1:
		/* This branch is unreachable. */
		printf("one\n");
		break;
	case 2:
		printf("two\n");
		break;
	}
}
