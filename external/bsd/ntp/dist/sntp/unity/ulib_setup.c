/*	$NetBSD: ulib_setup.c,v 1.1.1.1.16.2 2017/05/04 06:01:08 snj Exp $	*/

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
