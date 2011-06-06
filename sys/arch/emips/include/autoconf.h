/*	$NetBSD: autoconf.h,v 1.1.8.2 2011/06/06 09:05:17 jruoho Exp $ */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _EMIPS_AUTOCONF_H_
#define	_EMIPS_AUTOCONF_H_

/*
 * Machine-dependent structures for autoconfiguration
 */

struct mainbus_attach_args {
	const char *ma_name;		/* device name */
	int	    ma_slot;		/* CPU "slot" number; only meaningful
					   when attaching CPUs */
};

void	makebootdev __P((char *));

#endif	/* !_EMIPS_AUTOCONF_H_ */
