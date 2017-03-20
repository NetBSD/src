/*	$NetBSD: ulib_teardown.c,v 1.1.1.1.8.2 2017/03/20 10:56:58 martin Exp $	*/

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

