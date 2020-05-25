/*	$NetBSD: ulib_teardown.c,v 1.2 2020/05/25 20:47:35 christos Exp $	*/

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

