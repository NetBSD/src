/*	$NetBSD: cpu.c,v 1.7.12.1 2006/05/24 15:47:53 tron Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.7.12.1 2006/05/24 15:47:53 tron Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/autoconf.h>

#include "ioconf.h"

int	cpu_match(struct device *, struct cfdata *, void *);
void	cpu_attach(struct device *, struct device *, void *);

CFATTACH_DECL(cpu, sizeof (struct device),
    cpu_match, cpu_attach, NULL, NULL);

int
cpu_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return 1;
}

void
cpu_attach(struct device *parent, struct device *dev, void *aux)
{

	printf(": ");
	cpu_identify();
}
