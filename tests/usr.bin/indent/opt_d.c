/* $NetBSD: opt_d.c,v 1.2 2021/11/20 16:54:17 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the option '-d', which moves comments to the left of the
 * current code indentation level.
 */

#indent input
void
example(void)
/* comment at level 0 */
	/* comment at level 0 */
{
/* comment at level 1 */
	/* comment at level 1 */
		/* comment at level 1 */
	{
/* comment at level 2 */
	/* comment at level 2 */
		/* comment at level 2 */
			/* comment at level 2 */
	}
}
#indent end

/* The comments at level 0 move below the '{', due to the option '-br'. */
#indent run -d1
void
example(void)
{
/* comment at level 0 */
/* comment at level 0 */
/* comment at level 1 */
/* comment at level 1 */
/* comment at level 1 */
	{
	/* comment at level 2 */
	/* comment at level 2 */
	/* comment at level 2 */
	/* comment at level 2 */
	}
}
#indent end
