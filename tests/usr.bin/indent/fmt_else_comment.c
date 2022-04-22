/*	$NetBSD: fmt_else_comment.c,v 1.3 2022/04/22 21:21:20 rillig Exp $	*/

/*
 * Tests for comments after 'if (expr)' and 'else'. If the option '-br' is
 * given (or rather, if '-bl' is not given), indent looks ahead to the
 * following significant token to see whether it is a '{', it then moves the
 * comments after the '{'.
 *
 * See also:
 *	FreeBSD r303484
 *	FreeBSD r309342
 */

/*
 * The two 'if' statements below exercise two different code paths, even
 * though they look very similar.
 */
#indent input
void t(void) {
	if (1) /* a */ int a; else /* b */ int b;

	if (1) /* a */
		int a;
	else /* b */
		int b;
}
#indent end

#indent run
void
t(void)
{
	if (1)			/* a */
		int		a;
	else			/* b */
		int		b;

	if (1)			/* a */
		int		a;
	else			/* b */
		int		b;
}
#indent end


#indent input
void t(void) {
	if (1) {

	}



	/* Old indent would remove the 3 blank lines above, awaiting "else". */

	if (1) {
		int a;
	}


	else if (0) {
		int b;
	}
	/* test */
	else
		;

	if (1)
		;
	else /* Old indent would get very confused here */
	/* We also mustn't assume that there's only one comment */
	/* before the left brace. */
	{


	}
}
#indent end

#indent run -bl
void
t(void)
{
	if (1)
	{

	}



	/*
	 * Old indent would remove the 3 blank lines above, awaiting "else".
	 */

	if (1)
	{
		int		a;
	} else if (0)
	{
		int		b;
	}
	/* test */
	else
		;

	if (1)
		;
	else			/* Old indent would get very confused here */
		/* We also mustn't assume that there's only one comment */
		/* before the left brace. */
	{


	}
}
#indent end
