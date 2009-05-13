/*	$NetBSD: autoconf.h,v 1.6.92.1 2009/05/13 17:18:03 jym Exp $	*/

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

#ifndef _MIPSCO_AUTOCONF_H_
#define _MIPSCO_AUTOCONF_H_

/*
 * Machine-dependent structures for autoconfiguration
 */

#include <machine/bus.h>

struct confargs;

struct confargs {
	const char	*ca_name;	/* Device name. */
	int		ca_addr;	/* Device address. */
	int		ca_intr;	/* Device interrupt level */
	bus_space_tag_t	ca_bustag;	/* parent bus tag */
	bus_dma_tag_t	ca_dmatag;      /* parent bus dma */
};

/* Locator aliases */
#define cf_addr	cf_loc[OBIOCF_ADDR]

int	badaddr(void *, u_int);
void	makebootdev(char *);

#endif	/* !_MIPSCO_AUTOCONF_H_ */
