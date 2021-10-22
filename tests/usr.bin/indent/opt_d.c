/* $NetBSD: opt_d.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

/* XXX: oops, the comments at level 0 move below the '{' */
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
