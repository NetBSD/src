/*	$NetBSD: gapspci.c,v 1.1 2001/02/01 01:04:55 thorpej Exp $	*/

/*-
 * Copyright (c) 2001 Marcus Comstedt
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
 *	This product includes software developed by Marcus Comstedt.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/mbuf.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

#include <dreamcast/dev/g2/g2busvar.h>
#include <dreamcast/dev/g2/gapspcivar.h>

int	gaps_match(struct device *, struct cfdata *, void *);
void	gaps_attach(struct device *, struct device *, void *);

int	gaps_print(void *, const char *);

struct cfattach gapspci_ca = {
	sizeof(struct gaps_softc), 
	gaps_match,
	gaps_attach,
};

int
gaps_match(struct device *parent, struct cfdata *match, void *aux)
{
  /*  	struct g2bus_attach_args *ga = aux; */
	char idbuf[16];

	if(strcmp("gapspci", match->cf_driver->cd_name))
		return 0;

	/* FIXME */
	memcpy(idbuf, (void *)0xa1001400, sizeof(idbuf));

	if(strncmp(idbuf, "GAPSPCI_BRIDGE_2", 16))
		return 0;

	return (1);
}

void
gaps_attach(struct device *parent, struct device *self, void *aux)
{
	struct g2bus_attach_args *ga = aux;
	struct gaps_softc *sc = (void *) self;
	struct pcibus_attach_args pba;
	int i;

	printf(": SEGA GAPS PCI Bridge\n");

	sc->sc_dmabase = 0x1840000;
	sc->sc_memt = ga->ga_memt;

  /* FIXME */	
	*(volatile unsigned int *)(void *)(0xa1001418) = 0x5a14a501;

	for(i=0; i<1000000; i++)
	  ;
	
	if(*(volatile unsigned int *)(void *)(0xa1001418) != 1)
	  return;

	*(volatile unsigned int *)(void *)(0xa1001420) = 0x1000000;
	*(volatile unsigned int *)(void *)(0xa1001424) = 0x1000000;
	*(volatile unsigned int *)(void *)(0xa1001428) = sc->sc_dmabase;
	*(volatile unsigned int *)(void *)(0xa1001414) = 1;
	*(volatile unsigned int *)(void *)(0xa1001434) = 1;

	gaps_pci_init(sc);
	gaps_dma_init(sc);

	memset(&pba, 0, sizeof(pba));

	pba.pba_busname = "pci";
	pba.pba_memt = sc->sc_memt;
	pba.pba_dmat = &sc->sc_dmat;
	pba.pba_bus = 0;
	pba.pba_flags = PCI_FLAGS_MEM_ENABLED;
	pba.pba_pc = &sc->sc_pc;

	(void) config_found(self, &pba, gaps_print);
}

int
gaps_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);

	return (UNCONF);
}
