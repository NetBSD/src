/* $NetBSD: opt_cd.c,v 1.5 2023/06/17 22:09:24 rillig Exp $ */

/*
 * Tests for the '-cd' option, which sets the column (not the indentation) for
 * declarations.
 */

//indent input
int global_var; /* declaration comment */
stmt; /* declaration comment, as the code in this line starts at indentation level 0 */

{
	int local_var	/* unfinished declaration */
	;		/* finished declaration */
	stmt;		/* statement */
}
//indent end

//indent run -cd49
int		global_var;			/* declaration comment */
stmt;						/* declaration comment, as the
						 * code in this line starts at
						 * indentation level 0 */

{
	int		local_var		/* unfinished declaration */
// $ XXX: Why is the semicolon indented one column to the left?
		       ;			/* finished declaration */
	stmt;			/* statement */
}
//indent end

/* If '-cd' is not given, it falls back to '-c'. */
//indent run -c49
int		global_var;			/* declaration comment */
stmt;						/* declaration comment, as the
						 * code in this line starts at
						 * indentation level 0 */

{
	int		local_var		/* unfinished declaration */
// $ XXX: Why is the semicolon indented one column to the left?
		       ;			/* finished declaration */
	stmt;					/* statement */
}
//indent end
