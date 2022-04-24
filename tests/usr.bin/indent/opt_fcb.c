/* $NetBSD: opt_fcb.c,v 1.8 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the options '-fcb' and '-nfcb'.
 *
 * The option '-fcb' formats block comments (ones that begin with '/' '*'
 * '\n').
 *
 * The option '-nfcb' preserves block comments, like other box comments.
 */

/*
 * The following comment starts with '/' '*' '\n'.
 */
//indent input
/*
 * Block
 * comment
 * with delimiters.
 */
//indent end

//indent run -fcb
/*
 * Block comment with delimiters.
 */
//indent end

//indent run-equals-input -nfcb


/*
 * The following comment does not count as a block comment since it has a word
 * in its first line.
 */
//indent input
/* Not
 *
 * a block
 *      comment. */
//indent end

//indent run -fcb
/*
 * Not
 *
 * a block comment.
 */
//indent end

//indent run-equals-prev-output -nfcb


/*
 * Block comments that start with '-' or another '*' are always preserved.
 */
//indent input
/*-
 * car         mat         men
 *    efu   for   ted   com   t
 *       lly         box       .
 */
//indent end

//indent run-equals-input -fcb

//indent run-equals-input -nfcb


/*
 * The option '-fcb' does not distinguish between comments at the top level
 * and comments in functions.
 */
//indent input
void
example(void)
{
	/* Not
	 *
	 * a block
	 *      comment */
}
//indent end

//indent run -fcb
void
example(void)
{
	/*
	 * Not
	 *
	 * a block comment
	 */
}
//indent end

//indent run-equals-prev-output -nfcb


//indent input
void
example(void)
{
	/*
	 * This is
	 *
	 * a block
	 *	comment.
	 */
}
//indent end

//indent run -fcb
void
example(void)
{
	/*
	 * This is
	 *
	 * a block comment.
	 */
}
//indent end

//indent run-equals-input -nfcb


//indent input
void
example(void)
{
	/*-
	 * car         mat         men
	 *    efu   for   ted   com   t
	 *       lly         box       .
	 */
}
//indent end

//indent run-equals-input -fcb

//indent run-equals-input -nfcb
