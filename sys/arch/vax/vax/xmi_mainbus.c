/*	$NetBSD: xmi_mainbus.c,v 1.5 2003/07/15 02:15:06 lukem Exp $	   */
/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden.
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
__KERNEL_RCSID(0, "$NetBSD: xmi_mainbus.c,v 1.5 2003/07/15 02:15:06 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>

#define	_VAX_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/cpu.h>

#include <dev/xmi/xmivar.h>

static	int xmi_mainbus_match __P((struct device *, struct cfdata *, void *));
static	void xmi_mainbus_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(xmi_mainbus, sizeof(struct xmi_softc),
    xmi_mainbus_match, xmi_mainbus_attach, NULL, NULL);

extern	struct vax_bus_space vax_mem_bus_space;
extern	struct vax_bus_dma_tag vax_bus_dma_tag;

static int
xmi_mainbus_match(struct device *parent, struct cfdata *vcf, void *aux)
{
	if (vax_bustype == VAX_XMIBUS)
		return 1;
	return 0;
}

static void
xmi_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct xmi_softc *sc = (void *)self;

	/*
	 * Fill in bus specific data.
	 */
	sc->sc_addr = (bus_addr_t)0x21800000; /* XXX */
	sc->sc_iot = &vax_mem_bus_space; /* No special I/O handling */
	sc->sc_dmat = &vax_bus_dma_tag;	/* No special DMA handling either */
	sc->sc_intcpu = 1 << mastercpu;

	xmi_attach(sc);
}

void
xmi_intr_establish(void *icookie, int vec, void (*func)(void *), void *arg,
	struct evcnt *ev)
{
	scb_vecalloc(vec, func, arg, SCB_ISTACK, ev);
}
