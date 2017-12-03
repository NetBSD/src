/*	$NetBSD: wdc_extio.c,v 1.8.2.1 2017/12/03 11:36:26 jdolecek Exp $ */

/*-
 * Copyright (c) 2007 David Young.  All rights reserved.
 *
 * CompactFlash attachment for RouterBoard 153.  Derived from
 * the NetBSD/mac68k OBIO-IDE attachment.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2002 Takeshi Shibagaki  All rights reserved.
 *
 * mac68k OBIO-IDE attachment created by Takeshi Shibagaki
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: wdc_extio.c,v 1.8.2.1 2017/12/03 11:36:26 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <mips/adm5120/include/adm5120_extiovar.h>

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#ifdef WDC_EXTIO_DEBUG
int wdc_extio_debug = 1;
#define	WDC_EXTIO_DPRINTF(__fmt, ...)		\
do {						\
	if (wdc_extio_debug)			\
		printf((__fmt), __VA_ARGS__);	\
} while (/*CONSTCOND*/0)
#else /* !WDC_EXTIO_DEBUG */
#define	WDC_EXTIO_DPRINTF(__fmt, ...)	do { } while (/*CONSTCOND*/0)
#endif /* WDC_EXTIO_DEBUG */

#define	WDC_OBIO_CF_WINSIZE	0x1000
#define	WDC_OBIO_REG_OFFSET	0x0800
#define	WDC_OBIO_DATA_OFFSET	0x0808
#define	WDC_OBIO_AUXREG_OFFSET	0x080e

struct wdc_extio_softc {
	struct wdc_softc	sc_wdcdev;
	struct ata_channel	*sc_chanlist[1];
	struct ata_channel	sc_channel;
	struct wdc_regs		sc_wdc_regs;
	void			*sc_gpio;
	int			sc_map;
	struct gpio_pinmap	sc_pinmap;
};

int	wdc_extio_match(device_t, cfdata_t, void *);
void	wdc_extio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdc_extio, sizeof(struct wdc_extio_softc),
    wdc_extio_match, wdc_extio_attach, NULL, NULL);

#if 0
void	wdc_extio_reset(struct ata_channel *, int);
void
wdc_extio_reset(struct ata_channel *chp, int poll)
{
	struct wdc_softc *wdc = CHAN_TO_WDC(chp);
	struct wdc_regs *wdr = &wdc->regs[chp->ch_channel];
	uint8_t st0;
	int i, s = 0;

	if (poll)
		s = splbio();
	if (wdc->select)
		wdc->select(chp,0);
	/* assert SRST, wait for reset to complete */
	bus_space_write_1(wdr->ctl_iot, wdr->ctl_ioh, wd_aux_ctlr, WDCTL_RST);
	delay(2000);
	(void) bus_space_read_1(wdr->cmd_iot, wdr->cmd_iohs[wd_error], 0);
	bus_space_write_1(wdr->ctl_iot, wdr->ctl_ioh, wd_aux_ctlr, 0);
	delay(10);	/* 400ns delay */
	for (i = 3100000; --i > 0; ) {
		st0 = bus_space_read_1(wdr->cmd_iot,
		    wdr->cmd_iohs[wd_status], 0);
		if ((st0 & WDCS_BSY) == 0)
			break;
		delay(10);
	}
	if (i == 0)
		printf("%s: reset failed\n", __func__);
	else
		WDC_EXTIO_DPRINTF("%s: reset in %d.%02dms\n", __func__,
		    (3100000 - i) / 100,
		    (3100000 - i) % 100);

	if (poll) {
		/* ACK interrupt in case there is one pending left */
		if (wdc->irqack)
			wdc->irqack(chp);
		splx(s);
	}
}
#endif

static int
wdc_extio_map(struct extio_attach_args *ea, struct wdc_regs *wdr,
    struct ata_channel *chp, void **gpio, struct gpio_pinmap *pinmap)
{
	int i;

	*gpio = ea->ea_gpio;
	wdr->cmd_iot = wdr->ctl_iot = ea->ea_st;

#if 0
	if (gpio_pin_map(ea->ea_gpio, 0, ea->ea_gpio_mask, pinmap) != 0) {
		printf("%s: couldn't map GPIO\n", __func__);
		return -1;
	}
#endif

	if (bus_space_map(wdr->cmd_iot, ea->ea_addr, WDC_OBIO_CF_WINSIZE, 0,
	                  &wdr->cmd_baseioh)) {
		aprint_error("%s: couldn't map registers\n", __func__);
		goto post_gpio_err;
	}

	for (i = 1; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
		                        WDC_OBIO_REG_OFFSET + i, 1,
					&wdr->cmd_iohs[i]) != 0) {
			aprint_error(
			    "%s: unable to subregion control register\n",
			    __func__);
			goto post_bus_err;
		}
	}
	if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
				WDC_OBIO_DATA_OFFSET, 2,
				&wdr->cmd_iohs[wd_data]) != 0) {
		aprint_error("%s: unable to subregion data register\n",
		    __func__);
		goto post_bus_err;
	}
	if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
				WDC_OBIO_AUXREG_OFFSET, 1, &wdr->ctl_ioh)) {
		aprint_error("%s: unable to subregion aux register\n",
		    __func__);
		goto post_bus_err;
	}

	wdc_init_shadow_regs(chp);

	return 0;
post_bus_err:
	bus_space_unmap(wdr->cmd_iot, wdr->cmd_baseioh, WDC_OBIO_CF_WINSIZE);
post_gpio_err:
#if 0
	gpio_pin_unmap(ea->ea_gpio, pinmap);
#endif
	return -1;
}

static void
wdc_extio_dataout(struct ata_channel *chp, int flags, void *bf, size_t len)
{
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(chp);

	bus_space_write_multi_1(
	    wdr->cmd_iot, wdr->cmd_iohs[wd_data], 0, bf, len);
}

static void
wdc_extio_datain(struct ata_channel *chp, int flags, void *bf, size_t len)
{
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(chp);

	bus_space_read_multi_1(
	    wdr->cmd_iot, wdr->cmd_iohs[wd_data], 0, bf, len);
}

int
wdc_extio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct extio_attach_args *ea = (struct extio_attach_args *)aux;
	struct ata_channel ch;
	struct wdc_softc wdc;
	struct wdc_regs wdr;
	int result = 0;
	void *gpio;
	int map;
	struct gpio_pinmap pm = {.pm_map = &map};

	if (strcmp(ea->ea_name, cf->cf_name) != 0)
		return 0;

	WDC_EXTIO_DPRINTF("%s: enter\n", __func__);

	memset(&wdc, 0, sizeof(wdc));
	memset(&ch, 0, sizeof(ch));
	ch.ch_atac = &wdc.sc_atac;
	wdc.regs = &wdr;
	wdc.datain_pio = wdc_extio_datain;
	wdc.dataout_pio = wdc_extio_dataout;
	if (cf->cf_flags != -1) {
		wdc.sc_atac.atac_cap |= cf->cf_flags &
		    (ATAC_CAP_NOIRQ|ATAC_CAP_DATA32|ATAC_CAP_DATA16);
		wdc.cap |= cf->cf_flags &
		    (WDC_CAPABILITY_PREATA|WDC_CAPABILITY_NO_EXTRA_RESETS);
	}

	if (wdc_extio_map(ea, &wdr, &ch, &gpio, &pm) == -1)
		return 0;

	result = wdcprobe(&ch);

	bus_space_unmap(wdr.cmd_iot, wdr.cmd_baseioh, WDC_OBIO_CF_WINSIZE);
#if 0
	gpio_pin_unmap(ea->ea_gpio, &pm);
#endif

	return result;
}

void
wdc_extio_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_extio_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	struct extio_attach_args *ea = aux;
	struct ata_channel *chp = &sc->sc_channel;
	struct cfdata *cf;

	WDC_EXTIO_DPRINTF("%s: enter\n", __func__);

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_pinmap.pm_map = &sc->sc_map;
	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;
	chp->ch_atac = &sc->sc_wdcdev.sc_atac;

	if (wdc_extio_map(ea, wdr, chp, &sc->sc_gpio, &sc->sc_pinmap) == -1)
		return;

	cf = device_cfdata(sc->sc_wdcdev.sc_atac.atac_dev);
	if (cf->cf_flags != -1) {
		aprint_normal(" flags %04x", cf->cf_flags);
		sc->sc_wdcdev.sc_atac.atac_cap |= cf->cf_flags &
		    (ATAC_CAP_NOIRQ|ATAC_CAP_DATA32|ATAC_CAP_DATA16);
		sc->sc_wdcdev.cap |= cf->cf_flags &
		    (WDC_CAPABILITY_PREATA|WDC_CAPABILITY_NO_EXTRA_RESETS);
	}
	sc->sc_wdcdev.datain_pio = wdc_extio_datain;
	sc->sc_wdcdev.dataout_pio = wdc_extio_dataout;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_chanlist[0] = chp;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;

	chp->ch_channel = 0;
	chp->ch_atac = &sc->sc_wdcdev.sc_atac;

	aprint_normal("\n");

	wdcattach(chp);
}
