/*	$NetBSD: ulib_teardown.c,v 1.1.1.1.14.2 2017/05/04 05:53:55 snj Exp $	*/

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

