/*	$NetBSD: gapspci.c,v 1.17.2.1 2011/06/06 09:05:15 jruoho Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: gapspci.c,v 1.17.2.1 2011/06/06 09:05:15 jruoho Exp $");

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

int	gaps_match(device_t, cfdata_t, void *);
void	gaps_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gapspci, sizeof(struct gaps_softc),
    gaps_match, gaps_attach, NULL, NULL);

int
gaps_match(device_t parent, cfdata_t cf, void *aux)
{
	struct g2bus_attach_args *ga = aux;
	uint8_t idbuf[16];
	bus_space_handle_t tmp_memh;

	if (bus_space_map(ga->ga_memt, 0x01001400, 0x100, 0, &tmp_memh) != 0)
		return 0;

	bus_space_read_region_1(ga->ga_memt, tmp_memh, 0, idbuf, sizeof(idbuf));

	bus_space_unmap(ga->ga_memt, tmp_memh, 0x100);

	if (strncmp((const char *)idbuf, "GAPSPCI_BRIDGE_2", 16))
		return 0;

	return 1;
}

void
gaps_attach(device_t parent, device_t self, void *aux)
{
	struct g2bus_attach_args *ga = aux;
	struct gaps_softc *sc = device_private(self);
	struct pcibus_attach_args pba;
	int i;

	printf(": SEGA GAPS PCI Bridge\n");

	sc->sc_dev = self;
	sc->sc_memt = ga->ga_memt;

	sc->sc_dmabase = 0x1840000;
	sc->sc_dmasize = 32768;

	if (bus_space_map(sc->sc_memt, 0x01001400, 0x100,
	    0, &sc->sc_gaps_memh) != 0)
		panic("gaps_attach: can't map GAPS register space");

	bus_space_write_4(sc->sc_memt, sc->sc_gaps_memh, 0x18, 0x5a14a501);

	for (i = 0; i < 1000000; i++)
		;

	if (bus_space_read_4(sc->sc_memt, sc->sc_gaps_memh, 0x18) != 1)
		panic("gaps_attach: GAPS PCI bridge not responding");

	bus_space_write_4(sc->sc_memt, sc->sc_gaps_memh, 0x20, 0x1000000);
	bus_space_write_4(sc->sc_memt, sc->sc_gaps_memh, 0x24, 0x1000000);
	bus_space_write_4(sc->sc_memt, sc->sc_gaps_memh, 0x28, sc->sc_dmabase);
	bus_space_write_4(sc->sc_memt, sc->sc_gaps_memh, 0x2c,
	    sc->sc_dmabase + sc->sc_dmasize);
	bus_space_write_4(sc->sc_memt, sc->sc_gaps_memh, 0x14, 1);
	bus_space_write_4(sc->sc_memt, sc->sc_gaps_memh, 0x34, 1);

	gaps_pci_init(sc);
	gaps_dma_init(sc);

	memset(&pba, 0, sizeof(pba));

	pba.pba_memt = sc->sc_memt;
	pba.pba_dmat = &sc->sc_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_MEM_OKAY;
	pba.pba_pc = &sc->sc_pc;

	(void)config_found_ia(self, "pcibus", &pba, pcibusprint);
}
