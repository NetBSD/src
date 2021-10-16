/* $NetBSD: opt_fcb.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-fcb' and '-nfcb'.
 *
 * The option '-fcb' formats block comments (ones that begin with '/' '*'
 * '\n').
 *
 * The option '-nfcb' formats block comments like other box comments.
 */

/* FIXME: The options -fcb and -nfcb result in exactly the same output. */

#indent input
/* Not
 *
 * so carefully
 * formatted
 *      comment */

/*-
 * car         mat         men
 *    efu   for   ted   com   t
 *       lly         box       .
 */
#indent end

#indent run -fcb
/*
 * Not
 *
 * so carefully formatted comment
 */

/*-
 * car         mat         men
 *    efu   for   ted   com   t
 *       lly         box       .
 */
#indent end

#indent run -nfcb
/*
 * Not
 *
 * so carefully formatted comment
 */

/*-
 * car         mat         men
 *    efu   for   ted   com   t
 *       lly         box       .
 */
#indent end


#indent input
void
example(void)
{
	/* Not
	 *
	 * so carefully
	 * formatted
	 *      comment */

	/*-
	 * car         mat         men
	 *    efu   for   ted   com   t
	 *       lly         box       .
	 */
}
#indent end

#indent run -fcb
void
example(void)
{
	/*
	 * Not
	 *
	 * so carefully formatted comment
	 */

	/*-
	 * car         mat         men
	 *    efu   for   ted   com   t
	 *       lly         box       .
	 */
}
#indent end

#indent run -nfcb
void
example(void)
{
	/*
	 * Not
	 *
	 * so carefully formatted comment
	 */

	/*-
	 * car         mat         men
	 *    efu   for   ted   com   t
	 *       lly         box       .
	 */
}
#indent end
