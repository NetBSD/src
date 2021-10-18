/* $NetBSD: opt_cdb.c,v 1.4 2021/10/18 07:11:31 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-cdb' and '-ncdb'.
 *
 * The option '-cdb' forces the comment delimiter '/' '*' and '*' '/' to be on
 * a separate line. This only affects block comments, not comments to the
 * right of the code.
 *
 * The option '-ncdb' compresses multi-line comments to single-line comments,
 * as far as possible.
 */

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
#indent end

#indent run -cdb
/* A single-line comment. */

/*
 * A multi-line comment.
 */

/*
 * A multi-line comment.
 */
#indent end

#indent run -ncdb
/* A single-line comment. */

/* A multi-line comment. */

/* A multi-line comment. */
#indent end


/*
 * Code comments on global declarations.
 */
#indent input
int		ga;		/* A single-line comment. */

int		gb;		/* A
				 * multi-line
				 * comment. */

int		gc;		/*
				 * A
				 * multi-line
				 * comment.
				 */
#indent end

#indent run -cdb
int		ga;		/* A single-line comment. */

int		gb;		/* A multi-line comment. */

int		gc;		/* A multi-line comment. */
#indent end

#indent run-equals-prev-output -ncdb

/*
 * Block comments that are inside a function.
 */
#indent input
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
