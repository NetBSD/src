/* $NetBSD: isa_machdep.c,v 1.15.126.1 2009/05/13 17:16:05 jym Exp $ */

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

/*
 * Machine-specific functions for ISA autoconfiguration.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.15.126.1 2009/05/13 17:16:05 jym Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/isa/isavar.h>

#include "vga_isa.h"
#if NVGA_ISA
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/isa/vga_isavar.h>
#endif

#include "pcppi.h"
#if (NPCPPI > 0)
#include <dev/isa/pcppivar.h>

int isabeepmatch(struct device *, struct cfdata *, void *);
void isabeepattach(struct device *, struct device *, void *);

CFATTACH_DECL(isabeep, sizeof(struct device),
    isabeepmatch, isabeepattach, NULL, NULL);

static int ppi_attached;
static pcppi_tag_t ppicookie;
#endif /* PCPPI */

int
isa_display_console(bus_space_tag_t iot, bus_space_tag_t memt)
{
	int res = ENXIO;
#if NVGA_ISA
	res = vga_isa_cnattach(iot, memt);
	if (!res)
		return(0);
#endif
	return(res);
}

#if (NPCPPI > 0)
int
isabeepmatch(struct device *parent, struct cfdata *match, void *aux)
{
	return (!ppi_attached);
}

void
isabeepattach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");

	ppicookie = ((struct pcppi_attach_args *)aux)->pa_cookie;
	ppi_attached = 1;
}
#endif

void
isabeep(int pitch, int period)
{
#if (NPCPPI > 0)
	if (ppi_attached)
		pcppi_bell(ppicookie, pitch, period, 0);
#endif
}
