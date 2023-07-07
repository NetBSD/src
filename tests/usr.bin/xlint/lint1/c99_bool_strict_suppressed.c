/*	$NetBSD: c99_bool_strict_suppressed.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "c99_bool_strict_suppressed.c"

/*
 * In strict bool mode, like everywhere else, individual errors can be
 * suppressed.  Suppressing a message affects lint's output as well as the
 * exit status.  Lint's control flow stays the same as before though.
 *
 * This can result in assertion failures later.  One such assertion has been
 * there since at least 1995, at the beginning of expr(), ensuring that the
 * expression is either non-null or an error message has been _printed_.
 * In 1995, it was not possible to suppress error messages, which means that
 * the number of printed errors equaled the number of occurred errors.
 *
 * In err.c 1.12 from 2000-07-06, the option -X was added, allowing to
 * suppress individual error messages.  That commit did not mention any
 * interaction with the assertion in expr().  The assertion was removed in
 * tree.c 1.305 from 2021-07-04.
 */

/* lint1-extra-flags: -T -X 107,330,331,332,333 -X 351 */

/* ARGSUSED */
void
test(_Bool b, int i, const char *p)
{

	/* suppressed+1: error: controlling expression must be bool, not 'int' [333] */
	while (1)
		break;

	/* suppressed+1: error: operands of '=' have incompatible types '_Bool' and 'int' [107] */
	b = i;

	/* suppressed+1: error: operand of '!' must be bool, not 'int' [330] */
	b = !i;

	/* suppressed+1: error: left operand of '&&' must be bool, not 'int' [331] */
	b = i && b;

	/* suppressed+1: error: right operand of '&&' must be bool, not 'int' [332] */
	b = b && i;
}
