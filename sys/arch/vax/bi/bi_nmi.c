/*	$NetBSD: bi_nmi.c,v 1.5 2003/07/15 02:15:00 lukem Exp $	   */
/*
 * Copyright (c) 1999 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bi_nmi.c,v 1.5 2003/07/15 02:15:00 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#define	_VAX_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/cpu.h>
#include <machine/ka88.h>

#include <dev/bi/bivar.h>
#include <dev/bi/bireg.h>

#include "locators.h"

extern	struct vax_bus_space vax_mem_bus_space;
extern	struct vax_bus_dma_tag vax_bus_dma_tag;

static int
bi_nmi_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct nmi_attach_args *na = aux;

	if (cf->cf_loc[NMICF_SLOT] != NMICF_SLOT_DEFAULT &&
	    cf->cf_loc[NMICF_SLOT] != na->slot)
		return 0;
	if (na->slot < 10)
		return 1;
        return 0;
}

static void
bi_nmi_attach(struct device *parent, struct device *self, void *aux)
{
	struct nmi_attach_args *na = aux;
	struct bi_softc *sc = (void *)self;
	volatile int *v, *v2;
	extern int avail_end;
	int nid;

	/*
	 * Fill in bus specific data.
	 */
	sc->sc_addr = (bus_addr_t)BI_BASE(na->slot, 0);
	sc->sc_iot = &vax_mem_bus_space; /* No special I/O handling */
	sc->sc_dmat = &vax_bus_dma_tag;	/* No special DMA handling either */

	/* Must get the NBIB node number (interrupt routing) */
	/*
	 * XXX - need a big cleanup here!
	 */
	v = (int *)vax_map_physmem(NBIA_REGS(na->slot/2), 1);
	v[1] = v[1]; /* Clear errors */
	v[0] = 0x400 | CSR0_NBIIE | CSR0_LOOP; /* XXX */
	v2 = (int *)vax_map_physmem(sc->sc_addr, 1);
	v2[10] = v2[10] | 0x48;
	v2[8] = 0;
	v2[9] = (avail_end + 0x3ffff) & (~0x3ffff);
	v2[2] = v2[2];
	v2[1] = v2[1] | BICSR_BROKE;
	nid = v2[1] & 15;
	v[0] = (v[0] & ~CSR0_LOOP);
	DELAY(1000);

	sc->sc_intcpu = 1 << nid;
	sc->sc_lastiv = /* 0x400 * na->slot + */ 0x400;

	bi_attach(sc);
}

CFATTACH_DECL(bi_nmi, sizeof(struct bi_softc),
    bi_nmi_match, bi_nmi_attach, NULL, NULL);
