/* $NetBSD: opt_cdb.c,v 1.10 2023/06/09 07:18:52 rillig Exp $ */

/*
 * Tests for the options '-cdb' and '-ncdb'.
 *
 * The option '-cdb' forces the comment delimiter '/' '*' and '*' '/' to be on
 * a separate line. This only affects block comments, but not comments to the
 * right of the code.
 *
 * The option '-ncdb' preserves comments with delimiters.
 */

//indent input
/* A single line without delimiters. */

/* Multiple
 * lines
 * without delimiters. */

/*
 * A single line with delimiters.
 */

/*
 * Multiple
 * lines
 * with delimiters.
 */
//indent end

//indent run -cdb
/* A single line without delimiters. */

/*
 * Multiple lines without delimiters.
 */

/*
 * A single line with delimiters.
 */

/*
 * Multiple lines with delimiters.
 */
//indent end

//indent run -ncdb
/* A single line without delimiters. */

/* Multiple lines without delimiters. */

/*
 * A single line with delimiters.
 */

/*
 * Multiple lines with delimiters.
 */
//indent end


/*
 * Code comments on global declarations.
 */
//indent input
int global_single_without;	/* A single line without delimiters. */

int global_multi_without;	/*
				 * Multiple lines without delimiters.
				 */

int global_single_with;		/*
				 * A single line with delimiters.
				 */

int global_single_with;		/*
				 * Multiple
				 * lines
				 * with delimiters.
				 */
//indent end

//indent run -di0 -cdb
int global_single_without;	/* A single line without delimiters. */

int global_multi_without;	/* Multiple lines without delimiters. */

int global_single_with;		/* A single line with delimiters. */

int global_single_with;		/* Multiple lines with delimiters. */
//indent end

//indent run-equals-prev-output -di0 -ncdb


/*
 * Block comments that are inside a function.
 */
//indent input
void
example(void)
{
	/* A single line without delimiters. */
	int local_single_without;

	/* Multiple
	 * lines
	 * without delimiters. */
	int local_multi_without;

	/*
	 * A single line with delimiters.
	 */
	int local_single_with;

	/*
	 * Multiple
	 * lines
	 * with delimiters.
	 */
	int local_multi_with;
}
//indent end

//indent run -di0 -cdb
void
example(void)
{
	/* A single line without delimiters. */
	int local_single_without;

	/*
	 * Multiple lines without delimiters.
	 */
	int local_multi_without;

	/*
	 * A single line with delimiters.
	 */
	int local_single_with;

	/*
	 * Multiple lines with delimiters.
	 */
	int local_multi_with;
}
//indent end

//indent run -di0 -ncdb
void
example(void)
{
	/* A single line without delimiters. */
	int local_single_without;

	/* Multiple lines without delimiters. */
	int local_multi_without;

	/*
	 * A single line with delimiters.
	 */
	int local_single_with;

	/*
	 * Multiple lines with delimiters.
	 */
	int local_multi_with;
}
//indent end


//indent input
/*

 */
//indent end

//indent run -cdb
/*
 *
 */
//indent end

//indent run-equals-prev-output -ncdb


//indent input
/*

*/
//indent end

//indent run -cdb
/*
 *
 */
//indent end

//indent run-equals-prev-output -ncdb


/*
 * Between 2019-04-04 and 2023-06-09, the -ncdb option condensed multi-line
 * comments as well, not only single-line comments.
 */
//indent input
{
	/*
	 * This is the first paragraph.
	 *
	 * This is the second paragraph.
	 */
}
//indent end

//indent run-equals-input -cdb

//indent run-equals-input -ncdb
