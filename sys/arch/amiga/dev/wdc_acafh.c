/*	$NetBSD: wdc_acafh.c,v 1.6 2017/10/20 07:06:06 jdolecek Exp $ */

/*-
 * Copyright (c) 2000, 2003, 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael L. Hitch.
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

/*
 * ACA500 CF (IDE) controller driver. 
 *
 * The hardware emulates original A600/A1200 Gayle interface. However, it has
 * two channels, second channel placed instead of ctl registers (so software
 * reset of the bus is not possible, duh). There are no slave devices.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_acafh.c,v 1.6 2017/10/20 07:06:06 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <sys/bswap.h>

#include <amiga/amiga/cia.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/gayle.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/dev/acafhvar.h>
#include <amiga/dev/acafhreg.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#define WDC_ACAFH_SLOTS 2

struct wdc_acafh_slot {
	struct ata_channel	channel;
	struct wdc_regs		wdr;
};

struct wdc_acafh_softc {
	struct wdc_softc	sc_wdcdev;
	struct ata_channel	*sc_chanlist[WDC_ACAFH_SLOTS];
	struct wdc_acafh_slot	sc_slots[WDC_ACAFH_SLOTS];

	struct isr		sc_isr;

	struct bus_space_tag	cmd_iot;

	struct acafh_softc	*aca_sc;
};

int		wdc_acafh_match(device_t, cfdata_t, void *);
void		wdc_acafh_attach(device_t, device_t, void *);
int		wdc_acafh_intr(void *);
static void	wdc_acafh_attach_channel(struct wdc_acafh_softc *, int);
static void	wdc_acafh_map_channel(struct wdc_acafh_softc *, int);

CFATTACH_DECL_NEW(wdc_acafh, sizeof(struct wdc_acafh_softc),
    wdc_acafh_match, wdc_acafh_attach, NULL, NULL);

int
wdc_acafh_match(device_t parent, cfdata_t cfp, void *aux)
{
	struct acafhbus_attach_args *aap = aux;

	if (strcmp(aap->aaa_name, "wdc_acafh") != 0)
		return(0);
	return 1;
}

void
wdc_acafh_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_acafh_softc *sc = device_private(self);
	int i;

	aprint_normal(": ACA500 CompactFlash interface\n");
	sc->aca_sc = device_private(parent);

	gayle_init();

	/* XXX: take addr from attach args? */
	sc->cmd_iot.base = (u_long) ztwomap(GAYLE_IDE_BASE + 2);
	sc->cmd_iot.absm = &amiga_bus_stride_4swap;

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.sc_atac.atac_cap = ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = WDC_ACAFH_SLOTS;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->sc_wdcdev.cap = WDC_CAPABILITY_NO_AUXCTL;

	wdc_allocate_regs(&sc->sc_wdcdev);
	for (i = 0; i < WDC_ACAFH_SLOTS; i++) {
		wdc_acafh_attach_channel(sc, i);
	}

	sc->sc_isr.isr_intr = wdc_acafh_intr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr (&sc->sc_isr);

	gayle_intr_enable_set(GAYLE_INT_IDE);

}

void
wdc_acafh_attach_channel(struct wdc_acafh_softc *sc, int chnum)
{
#ifdef WDC_ACAFH_DEBUG
	device_t self;

	self = sc->sc_wdcdev.sc_atac.atac_dev;
#endif /* WDC_ACAFH_DEBUG */

	sc->sc_chanlist[chnum] = &sc->sc_slots[chnum].channel;
	memset(&sc->sc_slots[chnum],0,sizeof(struct wdc_acafh_slot));
	sc->sc_slots[chnum].channel.ch_channel = chnum;
	sc->sc_slots[chnum].channel.ch_atac = &sc->sc_wdcdev.sc_atac;

	wdc_acafh_map_channel(sc, chnum);

	wdc_init_shadow_regs(CHAN_TO_WDC_REGS(&sc->sc_slots[chnum].channel));
	wdcattach(&sc->sc_slots[chnum].channel);

#ifdef WDC_ACAFH_DEBUG
	aprint_normal_dev(self, "done init for channel %d\n", chnum);
#endif /* WDC_ACAFH_DEBUG */
}

void
wdc_acafh_map_channel(struct wdc_acafh_softc *sc, int chnum)
{
	struct wdc_regs *wdr;
	bus_addr_t off;
	device_t self;
	int i;

	self = sc->sc_wdcdev.sc_atac.atac_dev;

	wdr = CHAN_TO_WDC_REGS(&sc->sc_slots[chnum].channel);
	wdr->cmd_iot = &sc->cmd_iot;

	if (chnum == 0)
		off = 0;
	else
		off = 0x400;

	if (bus_space_map(wdr->cmd_iot, off, 0x40, 0,
			  &wdr->cmd_baseioh)) {
		aprint_error_dev(self, "couldn't map regs\n");
		return;
	}

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
		    wdr->cmd_baseioh, i, i == 0 ? 4 : 1,
		    &wdr->cmd_iohs[i]) != 0) {

			bus_space_unmap(wdr->cmd_iot,
			    wdr->cmd_baseioh, 0x40);
			aprint_error_dev(self, "couldn't map regs\n");
			return;
		}
	}

}

int
wdc_acafh_intr(void *arg)
{
	struct wdc_acafh_softc *sc;
	uint8_t intreq;
	int ret;

	sc = (struct wdc_acafh_softc *) arg;
	intreq = gayle_intr_status();
	ret = 0;

	if (intreq & GAYLE_INT_IDE) {
		if (acafh_cf_intr_status(sc->aca_sc, 1) == 1) {
			ret = wdcintr(&sc->sc_slots[1].channel);
		}
		if (acafh_cf_intr_status(sc->aca_sc, 0) == 1) {
			ret = wdcintr(&sc->sc_slots[0].channel);
		}
		gayle_intr_ack(0x7C | (intreq & GAYLE_INT_IDEACK));
	}

	return ret;
}

