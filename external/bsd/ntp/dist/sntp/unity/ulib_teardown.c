/*	$NetBSD: ulib_teardown.c,v 1.1.1.3 2020/05/25 20:40:15 christos Exp $	*/

/* default / lib implementation of 'tearDown()'
 *
 * SOLARIS does not support weak symbols -- need a real lib
 * implemetation here.
 */

extern void tearDown(void);

void tearDown(void)
{
	/* empty on purpose */
}

