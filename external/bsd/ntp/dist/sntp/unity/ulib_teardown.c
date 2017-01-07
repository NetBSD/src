/*	$NetBSD: ulib_teardown.c,v 1.1.1.1.2.2 2017/01/07 08:54:09 pgoyette Exp $	*/

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

