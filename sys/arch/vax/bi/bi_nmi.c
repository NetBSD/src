/*	$NetBSD: bi_nmi.c,v 1.8.18.1 2017/12/03 11:36:47 jdolecek Exp $	   */
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
__KERNEL_RCSID(0, "$NetBSD: bi_nmi.c,v 1.8.18.1 2017/12/03 11:36:47 jdolecek Exp $");

#define _VAX_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/ka88.h>

#include <dev/bi/bivar.h>
#include <dev/bi/bireg.h>

#include "locators.h"

static int
bi_nmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct nmi_attach_args * const na = aux;

	if (cf->cf_loc[NMICF_SLOT] != NMICF_SLOT_DEFAULT &&
	    cf->cf_loc[NMICF_SLOT] != na->na_slot)
		return 0;
	return na->na_slot < 10;
}

static void
bi_nmi_attach(device_t parent, device_t self, void *aux)
{
	struct nmi_attach_args * const na = aux;
	struct bi_softc * const sc = device_private(self);
	volatile int *v, *v2;
	extern int avail_end;
	int nid;

	sc->sc_dev = self;

	/*
	 * Fill in bus specific data.
	 */
	sc->sc_addr = (bus_addr_t)BI_BASE(na->na_slot, 0);
	sc->sc_iot = na->na_iot;	/* No special I/O handling */
	sc->sc_dmat = na->na_dmat;	/* No special DMA handling either */

	/* Must get the NBIB node number (interrupt routing) */
	/*
	 * XXX - need a big cleanup here!
	 */
	v = (int *)vax_map_physmem(NBIA_REGS(na->na_slot/2), 1);
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
	sc->sc_lastiv = /* 0x400 * na->na_slot + */ 0x400;

	bi_attach(sc);
}

CFATTACH_DECL_NEW(bi_nmi, sizeof(struct bi_softc),
    bi_nmi_match, bi_nmi_attach, NULL, NULL);
