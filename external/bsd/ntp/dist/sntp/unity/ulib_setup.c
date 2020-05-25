/*	$NetBSD: ulib_setup.c,v 1.1.1.3 2020/05/25 20:40:15 christos Exp $	*/

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
