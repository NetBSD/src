/*	$NetBSD: msg_217.c,v 1.4 2021/02/21 09:17:55 rillig Exp $	*/
# 3 "msg_217.c"

// Test for message: function %s falls off bottom without returning value [217]

int
random(int n)
{
	if (n < 0)
		return -3;
}				/* expect: 217 */

/*
 * The pattern 'do { } while (0)' is often used in statement macros.
 * Putting a 'return' at the end of such a macro is legitimate, the embracing
 * 'do { } while (0)' is probably there to conform to a coding standard or
 * to otherwise reduce confusion.
 *
 * Seen in external/bsd/libevent/dist/event_tagging.c, function
 * encode_int_internal.
 *
 * As of 2021-01-31, lint wrongly reports that the function would fall off
 * the bottom, but it cannot reach the bottom since every path contains the
 * 'return i'.
 */
int
do_while_return(int i)
{
	do {
		return i;
	} while (/*CONSTCOND*/0);	/*FIXME*//* expect: 193 */
}					/*FIXME*//* expect: 217 */

/*
 * C99 5.1.2.2.3 "Program termination" p1 defines that as a special exception,
 * the function 'main' does not have to return a value, reaching the bottom
 * is equivalent to returning 0.
 *
 * Before func.c 1.72 from 2021-02-21, lint had wrongly warned about this.
 */
int
main(void)
{
}
