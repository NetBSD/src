/* $NetBSD: opt_fcb.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

#indent input
/* FIXME: The options -fcb and -nfcb result in exactly the same output. */

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
/* FIXME: The options -fcb and -nfcb result in exactly the same output. */

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

#indent input
/* FIXME: The options -fcb and -nfcb result in exactly the same output. */

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

#indent run -nfcb
/* FIXME: The options -fcb and -nfcb result in exactly the same output. */

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
