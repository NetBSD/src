/*	$NetBSD: isabeep.c,v 1.8.26.1 2007/05/10 15:46:07 garbled Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
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
__KERNEL_RCSID(0, "$NetBSD: isabeep.c,v 1.8.26.1 2007/05/10 15:46:07 garbled Exp $");

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <dev/isa/isavar.h>

#include "pcppi.h"
#if NPCPPI > 0
#include <dev/isa/pcppivar.h>

int isabeepmatch(struct device *, struct cfdata *, void *);
void isabeepattach(struct device *, struct device *, void *);

CFATTACH_DECL(isabeep, sizeof(struct device),
    isabeepmatch, isabeepattach, NULL, NULL);

static int ppi_attached;
static pcppi_tag_t ppicookie;

int
isabeepmatch(struct device *parent, struct cfdata *match, void *aux)
{
	return (!ppi_attached);
}

void
isabeepattach(struct device *parent, struct device *self, void *aux)
{
	aprint_normal("\n");

	ppicookie = ((struct pcppi_attach_args *)aux)->pa_cookie;
	ppi_attached = 1;
}
#endif

void
isabeep(int pitch, int period)
{
#if NPCPPI > 0
	if (ppi_attached)
		pcppi_bell(ppicookie, pitch, period, 0);
#endif
}
