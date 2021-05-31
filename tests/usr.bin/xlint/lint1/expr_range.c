/*	$NetBSD: expr_range.c,v 1.2.2.2 2021/05/31 22:15:24 cjep Exp $	*/
# 3 "expr_range.c"

/*
 * In a switch statement that has (expr & constant) as the controlling
 * expression, complain if one of the case branches is unreachable because
 * the case label does can never match the controlling expression.
 *
 * GCC 10 does not complain about the unreachable branch.  It knows that the
 * branch is unreachable though since it doesn't generate any code for it.
 * GCC once had the option -Wunreachable-code, but that option was made a
 * no-op in 2011.
 *
 * Clang 10 does not complain about this either, and just like GCC it doesn't
 * generate any code for this branch.  The code for tracking an expression's
 * possible values may be related to RangeConstraintManager, just guessing.
 */

/* lint1-extra-flags: -chap */

void println(const char *);

void
example(unsigned x)
{
	switch (x & 6) {
	case 0:
		println("0 is reachable");
		break;
	case 1:			/* expect: statement not reached */
		println("1 is not reachable");
		break;
	case 2:
		println("2 is reachable");
		break;
	case 6:
		println("6 is reachable");
		break;
	case 7:			/* expect: statement not reached */
		println("7 is not reachable");
		break;
	}
}
