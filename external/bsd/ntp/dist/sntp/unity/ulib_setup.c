/*	$NetBSD: ulib_setup.c,v 1.1.1.1.2.2 2017/01/07 08:54:09 pgoyette Exp $	*/

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
