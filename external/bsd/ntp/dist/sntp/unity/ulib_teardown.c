/*	$NetBSD: ulib_teardown.c,v 1.1.1.1 2016/11/22 01:35:10 christos Exp $	*/

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

