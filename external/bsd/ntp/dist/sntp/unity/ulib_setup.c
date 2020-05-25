/*	$NetBSD: ulib_setup.c,v 1.2 2020/05/25 20:47:35 christos Exp $	*/

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
