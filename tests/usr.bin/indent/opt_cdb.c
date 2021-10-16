/* $NetBSD: opt_cdb.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
/* $FreeBSD$ */

#indent input
/* A single-line comment. */

/* A
 * multi-line
 * comment. */

/*
 * A
 * multi-line
 * comment.
 */

int		ga;		/* A single-line comment. */

int		gb;		/* A
				 * multi-line
				 * comment. */

int		gc;		/*
				 * A
				 * multi-line
				 * comment.
				 */

void
example(void)
{
	/* A single-line comment. */
	int la;

	/* A
	 * multi-line
	 * comment. */
	int lb;

	/*
	 * A
	 * multi-line
	 * comment.
	 */
	int lc;
}
#indent end

#indent run -cdb
/* A single-line comment. */

/*
 * A multi-line comment.
 */

/*
 * A multi-line comment.
 */

int		ga;		/* A single-line comment. */

int		gb;		/* A multi-line comment. */

int		gc;		/* A multi-line comment. */

void
example(void)
{
	/* A single-line comment. */
	int		la;

	/*
	 * A multi-line comment.
	 */
	int		lb;

	/*
	 * A multi-line comment.
	 */
	int		lc;
}
#indent end

#indent run -ncdb
/* A single-line comment. */

/* A multi-line comment. */

/* A multi-line comment. */

int		ga;		/* A single-line comment. */

int		gb;		/* A multi-line comment. */

int		gc;		/* A multi-line comment. */

void
example(void)
{
	/* A single-line comment. */
	int		la;

	/* A multi-line comment. */
	int		lb;

	/* A multi-line comment. */
	int		lc;
}
#indent end
