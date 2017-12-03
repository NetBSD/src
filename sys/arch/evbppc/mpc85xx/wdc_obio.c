/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: wdc_obio.c,v 1.4.2.1 2017/12/03 11:36:11 jdolecek Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/tty.h>

#include "ioconf.h"

#include <sys/intr.h>
#include <sys/bus.h>

#include <powerpc/booke/cpuvar.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

struct wdc_obio_softc {
	struct	wdc_softc sc_wdcdev;
	struct	ata_channel *wdc_chanlist[1];
	struct	ata_channel ata_channel;
	struct	wdc_regs wdc_regs;
	void	*sc_ih;
};

#define WDC_OBIO_AUXREG_OFFSET (WDC_NREG + 6)

static int wdc_obio_match(device_t, cfdata_t, void *);
static void wdc_obio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdc_obio, sizeof(struct wdc_obio_softc),
    wdc_obio_match, wdc_obio_attach, NULL, NULL);

static bool
wdc_obio_initregmap(struct wdc_regs *wdr, bus_space_tag_t bst,
	bus_addr_t addr, bus_size_t size)
{
	int error;

	wdr->cmd_iot = bst;
	wdr->ctl_iot = bst;

	error = bus_space_map(wdr->cmd_iot, addr, size, 0, &wdr->cmd_baseioh);
	if (error) {
		wdr->cmd_baseioh = 0;
		return false;
	}

	for (u_int i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
		    i, (i == 0) ? 2 : 1, &wdr->cmd_iohs[i])) {
			return false;
		}
	}

	if (bus_space_subregion(wdr->ctl_iot, wdr->cmd_baseioh,
	    WDC_OBIO_AUXREG_OFFSET, 1, &wdr->ctl_ioh)) {
		return false;
	}

	return true;
}

static int
wdc_obio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct generic_attach_args * const ga = aux;
	bus_size_t size = ga->ga_size;
	struct ata_channel ch;
	struct wdc_softc wdc;
	struct wdc_regs wdr;
	struct device dev;
	int rv = 0;

	if (ga->ga_addr == OBIOCF_ADDR_DEFAULT)
		return 0;
	if (size == OBIOCF_SIZE_DEFAULT  || size > PAGE_SIZE)
		size = 2 * WDC_NREG;

	/*
	 * We need to see if a CF is attached in True-IDE mode
	 */
	memset(&dev, 0, sizeof(dev));
	memset(&wdc, 0, sizeof(wdc));
	memset(&ch, 0, sizeof(ch));
	memset(&wdr, 0, sizeof(wdr));

	dev.dv_xname[0] = '?';
	wdc.sc_atac.atac_dev = &dev;
	ch.ch_atac = &wdc.sc_atac;
	wdc.regs = &wdr;

	if (wdc_obio_initregmap(&wdr, ga->ga_bst, ga->ga_addr, size)) {
		wdc_init_shadow_regs(&ch);
		rv = wdcprobe(&ch);
		bus_space_unmap(wdr.cmd_iot, wdr.cmd_baseioh, size);
	}

	return rv;
}

static void
wdc_obio_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_obio_softc *sc = device_private(self);
	struct wdc_regs * const wdr = &sc->wdc_regs;
	struct generic_attach_args * const ga = aux;
	bus_size_t size = ga->ga_size;

	if (size == OBIOCF_SIZE_DEFAULT || size > PAGE_SIZE)
		size = 2 * WDC_NREG;

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr;
	if (!wdc_obio_initregmap(wdr, ga->ga_bst, ga->ga_addr, size)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	//sc->sc_wdcdev.cap |= WDC_CAPABILITY_NO_EXTRA_RESETS;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->wdc_chanlist[0] = &sc->ata_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->ata_channel.ch_channel = 0;
	sc->ata_channel.ch_atac = &sc->sc_wdcdev.sc_atac;

	wdc_init_shadow_regs(wdr);

	/* 
	 * The interrupt line is controlled by a jumper.  We can't detect 
	 * this, except by allowing a request to time out, so assume that
	 * if the config file hasn't specified the IRQ, then the jumper isn't
	 * fitted.
	 */
	if (ga->ga_irq != OBIOCF_IRQ_DEFAULT) {
		sc->sc_ih = intr_establish(ga->ga_irq, IPL_BIO, IST_EDGE,
		    wdcintr, &sc->ata_channel);
		aprint_normal(": interrupting at irq %d\n", ga->ga_irq);
	} else {
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_NOIRQ;
		aprint_normal(": using polled I/O\n");
	}

	wdcattach(&sc->ata_channel);
}
