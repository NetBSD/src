/*	$NetBSD: wdc_mainbus.c,v 1.6 2017/10/20 07:06:07 jdolecek Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_mainbus.c,v 1.6 2017/10/20 07:06:07 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/mmeye.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include "locators.h"

#define	WDC_MAINBUS_REG_NPORTS		8
#define	WDC_MAINBUS_AUXREG_OFFSET	0x206
#define	WDC_MAINBUS_AUXREG_NPORTS	1

/* options passed via the 'flags' config keyword */
#define WDC_OPTIONS_32			0x01 /* try to use 32bit data I/O */
#define WDC_OPTIONS_ATA_NOSTREAM	0x04
#define WDC_OPTIONS_ATAPI_NOSTREAM	0x08

struct wdc_mainbus_softc {
	struct	wdc_softc sc_wdcdev;
	struct	ata_channel *wdc_chanlist[1];
	struct	ata_channel ata_channel;
	struct	wdc_regs wdc_regs;
};

static int wdc_mainbus_match(device_t, cfdata_t, void *);
static void wdc_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdc_mainbus, sizeof(struct wdc_mainbus_softc),
    wdc_mainbus_match, wdc_mainbus_attach, NULL, NULL);

static int
wdc_mainbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct ata_channel ch;
	struct wdc_softc wdc;
	struct wdc_regs wdr;
	int result = 0, i;

	if (strcmp(ma->ma_name, match->cf_name) != 0)
		return 0;

	/* Disallow wildcarded values. */
	if (ma->ma_addr1 == MAINBUSCF_ADDR1_DEFAULT ||
	    ma->ma_irq1 == MAINBUSCF_IRQ1_DEFAULT)
		return 0;

	memset(&wdc, 0, sizeof(wdc));
	memset(&ch, 0, sizeof(ch));
	ch.ch_atac = &wdc.sc_atac;
	wdc.regs = &wdr;

	wdr.cmd_iot = SH3_BUS_SPACE_PCMCIA_IO;
	if (bus_space_map(wdr.cmd_iot, ma->ma_addr1,
	    WDC_MAINBUS_REG_NPORTS, 0, &wdr.cmd_baseioh) != 0)
		goto out;

	for (i = 0; i < WDC_MAINBUS_REG_NPORTS; i++) {
		if (bus_space_subregion(wdr.cmd_iot, wdr.cmd_baseioh, i,
		    i == 0 ? 4 : 1, &wdr.cmd_iohs[i]) != 0)
			goto outunmap;
	}
	wdc_init_shadow_regs(&ch);

	wdr.ctl_iot = SH3_BUS_SPACE_PCMCIA_IO;
	if (bus_space_map(wdr.ctl_iot, ma->ma_addr1 + WDC_MAINBUS_AUXREG_OFFSET,
	    WDC_MAINBUS_AUXREG_NPORTS, 0, &wdr.ctl_ioh) != 0)
		goto outunmap;

#if 0
bus_space_write_1(iot, ioh, 0x200, 0x80);
delay(1000);
bus_space_write_1(iot, ioh, 0x200, 0x00);
delay(1000);
printf("CF COR=0x%x\n", bus_space_read_1(iot, ioh, 0x200));
bus_space_write_1(iot, ioh, 0x200, 0x41);
printf("CF COR=0x%x\n", bus_space_read_1(iot, ioh, 0x200));
delay(1000000);
#endif
	result = wdcprobe(&ch);

	bus_space_unmap(wdr.ctl_iot, wdr.ctl_ioh, WDC_MAINBUS_AUXREG_NPORTS);
outunmap:
	bus_space_unmap(wdr.cmd_iot, wdr.cmd_baseioh, WDC_MAINBUS_REG_NPORTS);
out:
	return result;
}

static void
wdc_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_mainbus_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	struct wdc_regs *wdr;
	int wdc_cf_flags = device_cfdata(self)->cf_flags;
	int i;

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->wdc_regs;
	wdr->cmd_iot = SH3_BUS_SPACE_PCMCIA_IO;
	wdr->ctl_iot = SH3_BUS_SPACE_PCMCIA_IO;
	if (bus_space_map(wdr->cmd_iot, ma->ma_addr1,
	      WDC_MAINBUS_REG_NPORTS, 0, &wdr->cmd_baseioh) ||
	    bus_space_map(wdr->ctl_iot,
	      ma->ma_addr1 + WDC_MAINBUS_AUXREG_OFFSET,
	      WDC_MAINBUS_AUXREG_NPORTS, 0, &wdr->ctl_ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	for (i = 0; i < WDC_MAINBUS_REG_NPORTS; i++) {
		if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh, i,
		    i == 0 ? 4 : 1, &wdr->cmd_iohs[i]) != 0) {
			aprint_error(": couldn't subregion registers (3)\n");
			return;
		}
	}

	wdr->data32iot = wdr->cmd_iot;
	wdr->data32ioh = wdr->cmd_iohs[0];

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_PREATA;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
	if (wdc_cf_flags & WDC_OPTIONS_32)
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA32;
	if (wdc_cf_flags & WDC_OPTIONS_ATA_NOSTREAM)
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_ATA_NOSTREAM;
	if (wdc_cf_flags & WDC_OPTIONS_ATAPI_NOSTREAM)
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_ATAPI_NOSTREAM;

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->wdc_chanlist[0] = &sc->ata_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->ata_channel.ch_channel = 0;
	sc->ata_channel.ch_atac = &sc->sc_wdcdev.sc_atac;

	wdc_init_shadow_regs(wdr);

	aprint_normal("\n");

	mmeye_intr_establish(ma->ma_irq1, IST_LEVEL, IPL_BIO,
	    wdcintr, &sc->ata_channel);

	wdcattach(&sc->ata_channel);
}
