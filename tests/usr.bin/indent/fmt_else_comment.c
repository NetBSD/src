/*	$NetBSD: fmt_else_comment.c,v 1.6 2023/06/23 20:44:51 rillig Exp $	*/

/*
 * Tests for comments after 'if (expr)' and 'else'. Before 2023-05-11, if the
 * option '-br' was given (or rather, if '-bl' was not given), indent looked
 * ahead to the following significant token to see whether it was a '{', it
 * then moved the comments after the '{'. This token swapping was error-prone
 * and thus removed.
 *
 * See also:
 *	FreeBSD r303484
 *	FreeBSD r309342
 */

/*
 * Before 2023-05-11, the two 'if' statements below exercised two different
 * code paths, even though they look very similar.
 */
//indent input
void t(void) {
	if (1) /* a */ int a; else /* b */ int b;

	if (1) /* a */
		int a;
	else /* b */
		int b;
}
//indent end

//indent run
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
//indent end


//indent input
void t(void) {
	if (1) {

	}



	/* Old indent would remove the 3 blank lines above, awaiting "else". */
	// $ 'Old' means something before 2019.

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
	// $ 'Old' means something before 2019.
	/* We also mustn't assume that there's only one comment */
	/* before the left brace. */
	{


	}
}
//indent end

//indent run -bl
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
	}


	else if (0)
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
//indent end
