/*	$NetBSD: autoconf.h,v 1.4.136.1 2011/06/06 09:04:42 jruoho Exp $	*/

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

#ifndef _ALGOR_AUTOCONF_H_
#define	_ALGOR_AUTOCONF_H_

#include <machine/bus.h>

/*
 * Machine-dependent structures for autoconfiguration
 */
struct mainbus_attach_args {
	const char *ma_name;		/* device name */
	bus_space_tag_t ma_st;		/* the space tag to use */
	bus_addr_t ma_addr;		/* system bus address */
	int	   ma_irq;		/* IRQ index */
};

#ifdef _KERNEL
extern char algor_ethaddr[];

void	(*algor_iointr)(int, vaddr_t, uint32_t);

void	led_display(uint8_t, uint8_t, uint8_t, uint8_t);
#endif /* _KERNEL */

#endif	/* !_ALGOR_AUTOCONF_H_ */
