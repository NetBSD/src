/* $NetBSD: wdc_g1.c,v 1.3.2.2 2017/12/03 11:36:00 jdolecek Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Onno van der Linden.
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

#include "opt_ata.h"	/* for ATADEBUG */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/bus.h>

#include <machine/intr.h>
#include <machine/sysasicvar.h>

#include <arch/dreamcast/dev/g1/g1busvar.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>
#include <dev/ata/atareg.h>

#define WDC_G1_CMD_ADDR			0x005f7080
#define WDC_G1_REG_NPORTS		8
#define WDC_G1_CTL_ADDR			0x005f7018
#define WDC_G1_AUXREG_NPORTS		1

struct wdc_g1_softc {
	struct	wdc_softc sc_wdcdev;
	struct	ata_channel *wdc_chanlist[1];
	struct	ata_channel ata_channel;
	struct	wdc_regs wdc_regs;
	void	*sc_ih;
	int	sc_irq;
};

static int	wdc_g1_probe(device_t, cfdata_t, void *);
static void	wdc_g1_attach(device_t, device_t, void *);
static void	wdc_g1_do_reset(struct ata_channel *, int);
static int	wdc_g1_intr(void *);

CFATTACH_DECL_NEW(wdc_g1bus, sizeof(struct wdc_g1_softc),
    wdc_g1_probe, wdc_g1_attach, NULL, NULL);

static int
wdc_g1_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct ata_channel ch;
	struct g1bus_attach_args *ga = aux;
	struct wdc_softc wdc;
	struct wdc_regs wdr;
	int result = 0, i;
#ifdef ATADEBUG
	struct device dev;
#endif

	*((volatile uint32_t *)0xa05f74e4) = 0x1fffff;
	for (i = 0; i < 0x200000 / 4; i++)
		(void)((volatile uint32_t *)0xa0000000)[i];

	memset(&wdc, 0, sizeof(wdc));
	memset(&ch, 0, sizeof(ch));
	ch.ch_atac = &wdc.sc_atac;
	wdc.reset = wdc_g1_do_reset;
	wdc.regs = &wdr;

	wdr.cmd_iot = ga->ga_memt;
	if (bus_space_map(wdr.cmd_iot, WDC_G1_CMD_ADDR,
	    WDC_G1_REG_NPORTS * 4, 0, &wdr.cmd_baseioh))
		goto out;

	for (i = 0; i < WDC_G1_REG_NPORTS; i++) {
		if (bus_space_subregion(wdr.cmd_iot, wdr.cmd_baseioh, i * 4,
		    i == 0 ? 2 : 1, &wdr.cmd_iohs[i]) != 0)
			goto outunmap;
	}

	wdc_init_shadow_regs(&ch);

	wdr.ctl_iot = ga->ga_memt;
	if (bus_space_map(wdr.ctl_iot, WDC_G1_CTL_ADDR,
	    WDC_G1_AUXREG_NPORTS, 0, &wdr.ctl_ioh))
	  goto outunmap;

#ifdef ATADEBUG
	/* fake up device name for ATADEBUG_PRINT() with DEBUG_PROBE */
	memset(&dev, 0, sizeof(dev));
	strncat(dev.dv_xname, "wdc(g1probe)", sizeof(dev.dv_xname));
	wdc.sc_atac.atac_dev = &dev;
#endif
	result = wdcprobe(&ch);
	
	bus_space_unmap(wdr.ctl_iot, wdr.ctl_ioh, WDC_G1_AUXREG_NPORTS);
 outunmap:
	bus_space_unmap(wdr.cmd_iot, wdr.cmd_baseioh, WDC_G1_REG_NPORTS);
 out:
	return result;
}

static void
wdc_g1_attach(struct device *parent, struct device *self, void *aux)
{
	struct wdc_g1_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	struct g1bus_attach_args *ga = aux;
	int i;

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->wdc_regs;

	wdr->cmd_iot = ga->ga_memt;
	wdr->ctl_iot = ga->ga_memt;
	if (bus_space_map(wdr->cmd_iot, WDC_G1_CMD_ADDR,
	    WDC_G1_REG_NPORTS * 4, 0, &wdr->cmd_baseioh) ||
	    bus_space_map(wdr->ctl_iot, WDC_G1_CTL_ADDR,
	    WDC_G1_AUXREG_NPORTS, 0, &wdr->ctl_ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	for (i = 0; i < WDC_G1_REG_NPORTS; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
		      wdr->cmd_baseioh, i * 4, i == 0 ? 2 : 1,
		      &wdr->cmd_iohs[i]) != 0) {
			aprint_error(": couldn't subregion registers\n");
			return;
		}
	}

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_PREATA;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->wdc_chanlist[0] = &sc->ata_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->sc_wdcdev.reset = wdc_g1_do_reset;
	sc->ata_channel.ch_channel = 0;
	sc->ata_channel.ch_atac = &sc->sc_wdcdev.sc_atac;

	wdc_init_shadow_regs(wdr);

	aprint_normal(": %s\n", sysasic_intr_string(SYSASIC_IRL9));

	sysasic_intr_establish(SYSASIC_EVENT_GDROM, IPL_BIO, SYSASIC_IRL9,
	    wdc_g1_intr, &sc->ata_channel);

	wdcattach(&sc->ata_channel);
}

int
wdc_g1_intr(void *arg)
{

	return wdcintr(arg);
}

static void
wdc_g1_do_reset(struct ata_channel *chp, int poll)
{
	struct wdc_softc *wdc = CHAN_TO_WDC(chp);
	struct wdc_regs *wdr = &wdc->regs[chp->ch_channel];
	int s = 0;

	if (poll != 0)
		s = splbio();

	/* master */
	bus_space_write_1(wdr->cmd_iot, wdr->cmd_iohs[wd_sdh], 0,
	    WDSD_IBM);
	delay(10);	/* 400ns delay */
	/* assert SRST, wait for reset to complete */
	bus_space_write_1(wdr->ctl_iot, wdr->ctl_ioh, wd_aux_ctlr,
	    WDCTL_RST | WDCTL_4BIT | WDCTL_IDS);
	delay(2000);
	(void) bus_space_read_1(wdr->cmd_iot, wdr->cmd_iohs[wd_error], 0);
	bus_space_write_1(wdr->ctl_iot, wdr->ctl_ioh, wd_aux_ctlr,
	    WDCTL_4BIT | WDCTL_IDS);
	delay(10);	/* 400ns delay */

	/* reset GD-ROM at master via ATAPI command */
	bus_space_write_1(wdr->cmd_iot, wdr->cmd_iohs[wd_sdh], 0,
	    WDSD_IBM);
	bus_space_write_1(wdr->cmd_iot, wdr->cmd_iohs[wd_command], 0,
	    ATAPI_SOFT_RESET);
	delay(100 * 1000);

	if (poll != 0)
		splx(s);
}
