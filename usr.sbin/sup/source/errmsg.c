/*	$NetBSD: errmsg.c,v 1.8 2003/04/03 17:14:24 christos Exp $	*/

/*
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the rights
 * to redistribute these changes.
 */
/*****************************************************************
 * HISTORY
 * 04-Mar-85  Rudy Nedved (ern) at Carnegie-Mellon University
 *	Create a CMU version of the BBN errmsg routine from scratch. It
 *	differs from the BBN errmsg routine in the fact that it uses a
 *	negative value to indicate using the current errno value...the
 *	BBN uses a negative OR zero value.
 */

#include "supcdefs.h"
#include "supextern.h"

#ifndef __NetBSD__
static char *itoa(char *, unsigned);

static char *
itoa(char *p, unsigned n)
{
	if (n >= 10)
		p = itoa(p, n / 10);
	*p++ = (n % 10) + '0';
	return (p);
}
#endif

const char *
errmsg(int cod)
{
#ifndef errno
	extern int errno;
	extern int sys_nerr;
	extern char *sys_errlist[];
	static char unkmsg[] = "Unknown error ";
	static char unk[sizeof(unkmsg) + 11];	/* trust us */
#endif

	if (cod < 0)
		cod = errno;

#ifndef errno
	if ((cod >= 0) && (cod < sys_nerr))
		return (sys_errlist[cod]);

	strcpy(unk, unkmsg);
	*itoa(&unk[sizeof(unkmsg) - 1], cod) = '\0';

	return (unk);
#else
	return strerror(cod);
#endif
}
