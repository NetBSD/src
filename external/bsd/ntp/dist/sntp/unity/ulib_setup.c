/*	$NetBSD: ulib_setup.c,v 1.1.1.1.10.2 2017/03/20 10:59:49 martin Exp $	*/

/* default / lib implementation of 'setUp()'
 *
 * SOLARIS does not support weak symbols -- need a real lib
 * implemetation here.
 */

extern void setUp(void);

void setUp(void)
{
	/* empty on purpose */
}

/* -*- that's all folks! -*- */
