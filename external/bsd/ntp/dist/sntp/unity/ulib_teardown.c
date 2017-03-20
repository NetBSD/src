/*	$NetBSD: ulib_teardown.c,v 1.1.1.1.10.2 2017/03/20 10:59:49 martin Exp $	*/

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

