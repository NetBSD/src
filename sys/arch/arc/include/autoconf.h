/*	$NetBSD: autoconf.h,v 1.13.90.1 2011/06/06 09:04:59 jruoho Exp $	*/
/*	$OpenBSD: autoconf.h,v 1.2 1997/03/12 19:16:54 pefo Exp $	*/
/*	NetBSD: autoconf.h,v 1.1 1995/02/13 23:07:31 cgd Exp 	*/

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

/*
 * Machine-dependent structures for autoconfiguration
 */

#ifndef _ARC_AUTOCONF_H_
#define _ARC_AUTOCONF_H_

struct confargs;

typedef int (*intr_handler_t)(void *);

struct confargs {
	const char *ca_name;		/* Device name. */
	int	ca_slot;		/* Device slot. */
	int	ca_offset;		/* Offset into slot. */
};

void	makebootdev(const char *cp);

/* serial console related variables */
extern int com_freq;
extern int com_console;
extern int com_console_address;
extern int com_console_speed;
extern int com_console_mode;
#endif /* _ARC_AUTOCONF_H_ */
