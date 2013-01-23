/*	$NetBSD: omap3_sdma.c,v 1.2.2.2 2013/01/23 00:05:43 yamt Exp $	*/

/*
 * Copyright (c) 2012 Michael Lorenz
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

/*
 * OMAP 3530 DMA controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap3_sdma.c,v 1.2.2.2 2013/01/23 00:05:43 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/lwp.h>
#include <sys/kauth.h>

#include <sys/bus.h>
#include <arm/omap/omap3_sdmareg.h>
#include <arm/omap/omap3_sdmavar.h>
#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_obioreg.h>
#include <arm/omap/omap2_prcm.h>

struct omapdma_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_regh;
};

static int	omapdma_match(device_t, cfdata_t, void *);
static void	omapdma_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(omapdma, sizeof(struct omapdma_softc),
    omapdma_match, omapdma_attach, NULL, NULL);

struct omapdma_softc *omapdma_sc = NULL;

static int
omapdma_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *obio = aux;

	if ((obio->obio_addr == -1) || (obio->obio_size == 0))
		return 0;
	return 1;
}

static void
omapdma_attach(device_t parent, device_t self, void *aux)
{
	struct omapdma_softc	*sc = device_private(self);
	struct obio_attach_args *obio = aux;
	uint32_t reg;

	sc->sc_iot = obio->obio_iot;
	sc->sc_dev = self;
	
	printf(": OMAP DMA controller rev ");
	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size, 0,
	    &sc->sc_regh)) {
		aprint_error_dev(sc->sc_dev, ": couldn't map register space\n");
		return;
	}
	reg = bus_space_read_4(sc->sc_iot, sc->sc_regh, OMAPDMA_REVISION);
	printf("%d.%d\n", (reg >> 4) & 0xf, reg & 0xf);

	/* disable & clear all interrupts etc. */
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPDMA_IRQENABLE_L0, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPDMA_IRQENABLE_L1, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPDMA_IRQENABLE_L2, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPDMA_IRQENABLE_L3, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_regh,
	    OMAPDMA_IRQSTATUS_L0, 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_regh,
	    OMAPDMA_IRQSTATUS_L1, 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_regh,
	    OMAPDMA_IRQSTATUS_L2, 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_regh,
	    OMAPDMA_IRQSTATUS_L3, 0xffffffff);

	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPDMA_SYSCONFIG,
	    OMAPDMA_IDLEMODE_SMART_STANDBY | OMAPDMA_SMART_IDLE | 
	    OMAPDMA_AUTOIDLE);

	/* allow more FIFO space for large bursts used by omapfb */
	bus_space_write_4(sc->sc_iot, sc->sc_regh, OMAPDMA_GCR, 
		(1 << OMAPDMA_GCR_ARB_RATE_SHIFT) | 0x80);

	omapdma_sc = sc;

	/*
	 * TODO:
	 * - channel allocation
	 */
}

void
omapdma_write_reg(int reg, uint32_t val)
{
	if (omapdma_sc != NULL) {
		bus_space_write_4(omapdma_sc->sc_iot, omapdma_sc->sc_regh,
		    reg, val);
	}
}

uint32_t
omapdma_read_reg(int reg)
{
	if (omapdma_sc != NULL) {
		return bus_space_read_4(omapdma_sc->sc_iot,
		    omapdma_sc->sc_regh, reg);
	}
	return 0;
}

void
omapdma_write_ch_reg(int ch, int reg, uint32_t val)
{
	if (omapdma_sc != NULL) {
		bus_space_write_4(omapdma_sc->sc_iot, omapdma_sc->sc_regh,
		    OMAPDMA_CHANNEL_BASE + 0x60 * ch + reg, val); 
	}
}

uint32_t
omapdma_read_ch_reg(int ch, int reg)
{
	if (omapdma_sc != NULL) {
		return bus_space_read_4(omapdma_sc->sc_iot,
		    omapdma_sc->sc_regh,
		    OMAPDMA_CHANNEL_BASE + 0x60 * ch + reg); 
	}
	return 0;
}
