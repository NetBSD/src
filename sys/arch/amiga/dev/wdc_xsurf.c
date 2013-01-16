/*      $NetBSD: wdc_xsurf.c,v 1.2.2.2 2013/01/16 05:32:42 yamt Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/* Driver for IDE controller present on X-Surf. */

#include <sys/cdefs.h>

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kmem.h>

#include <machine/cpu.h>

#include <amiga/amiga/device.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/dev/xsurfvar.h>

#define WDC_XSURF_CHANNELS	2
#define WDC_XSURF_CHANSIZE	0x30
#define WDC_XSURF_CHANOFF	0x2000

#define WDC_XSURF_ISR_OFF	0x7E
#define WDC_XSURF_ISR_HIGH	0x80

#define WDC_XSURF_OFF_DATA	0x0
#define WDC_XSURF_OFF_ERROR	0x6
#define WDC_XSURF_OFF_SECCNT	0xA
#define WDC_XSURF_OFF_SECTOR	0xE
#define WDC_XSURF_OFF_CYL_LO	0x12
#define WDC_XSURF_OFF_CYL_HI	0x16
#define WDC_XSURF_OFF_SDH	0x1A
#define WDC_XSURF_OFF_COMMAND	0x1E

struct wdc_xsurf_port {
	struct ata_channel	channel;
	struct ata_queue	queue;
	struct wdc_regs		wdr;
};

struct wdc_xsurf_softc {
	device_t		sc_dev;	

	struct wdc_softc 	sc_wdcdev;
	struct ata_channel 	*sc_chanarray[WDC_XSURF_CHANNELS];
	struct wdc_xsurf_port	sc_ports[WDC_XSURF_CHANNELS];

	struct bus_space_tag	sc_bst;
	bus_space_tag_t		sc_cmdt;
	bus_space_handle_t	sc_isrh;

	struct isr		sc_isr;
};

static int	wdc_xsurf_match(device_t, cfdata_t, void *);
static void	wdc_xsurf_attach(device_t, device_t, void *);
void		wdc_xsurf_attach_channel(struct wdc_xsurf_softc *, int);
void		wdc_xsurf_map_channel(struct wdc_xsurf_softc *, int);
int		wdc_xsurf_intr(void *arg);

CFATTACH_DECL_NEW(wdc_xsurf, sizeof(struct wdc_softc),
    wdc_xsurf_match, wdc_xsurf_attach, NULL, NULL);

static const unsigned int wdc_xsurf_wdr_offsets[] = {
    WDC_XSURF_OFF_DATA, WDC_XSURF_OFF_ERROR, WDC_XSURF_OFF_SECCNT,
    WDC_XSURF_OFF_SECTOR, WDC_XSURF_OFF_CYL_LO, WDC_XSURF_OFF_CYL_HI,
    WDC_XSURF_OFF_SDH, WDC_XSURF_OFF_COMMAND
};

static int
wdc_xsurf_match(device_t parent, cfdata_t cf, void *aux)
{
	struct xsurfbus_attach_args *xsb_aa = aux;

	if (strcmp(xsb_aa->xaa_name, "wdc_xsurf") != 0) 
		return 0;

	return 1;
}

static void
wdc_xsurf_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_xsurf_softc *sc;
	struct xsurfbus_attach_args *xsb_aa;
	int i;

	aprint_normal("\n");
	aprint_naive("\n");
	xsb_aa = aux;
	sc = device_private(self);
	sc->sc_dev = self;

	sc->sc_bst.base = xsb_aa->xaa_base;
	sc->sc_bst.absm = &amiga_bus_stride_1swap;
	sc->sc_cmdt = &sc->sc_bst;

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_wdcdev.sc_atac.atac_nchannels = WDC_XSURF_CHANNELS;
	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanarray;
	sc->sc_wdcdev.sc_atac.atac_cap = ATAC_CAP_DATA16;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	/* this controller has no aux control registers */
	sc->sc_wdcdev.cap = WDC_CAPABILITY_NO_AUXCTL;

	/* attach the channels */
	wdc_allocate_regs(&sc->sc_wdcdev);
	for (i = 0; i < WDC_XSURF_CHANNELS; i++) {
		wdc_xsurf_attach_channel(sc, i);
	}

	/* map interrupt status register */
	bus_space_map(sc->sc_cmdt, WDC_XSURF_ISR_OFF, 
	    1, 0, &sc->sc_isrh);
	
	/* attach interrupt */
	sc->sc_isr.isr_intr = wdc_xsurf_intr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr(&sc->sc_isr);
}

void
wdc_xsurf_attach_channel(struct wdc_xsurf_softc *sc, int chnum)
{
	sc->sc_chanarray[chnum] = &sc->sc_ports[chnum].channel;
	memset(&sc->sc_ports[chnum],0,sizeof(struct wdc_xsurf_port));
	sc->sc_ports[chnum].channel.ch_channel = chnum;
	sc->sc_ports[chnum].channel.ch_atac = &sc->sc_wdcdev.sc_atac;
	sc->sc_ports[chnum].channel.ch_queue = &sc->sc_ports[chnum].queue;

	wdc_xsurf_map_channel(sc, chnum);	

	wdc_init_shadow_regs(&sc->sc_ports[chnum].channel);
	wdcattach(&sc->sc_ports[chnum].channel);

#ifdef WDC_XSURF_DEBUG
	aprint_normal_dev(sc->sc_dev, "done init for channel %d\n", chnum);
#endif /* WDC_XSURF_DEBUG */
}

void
wdc_xsurf_map_channel(struct wdc_xsurf_softc *sc, int chnum)
{
	struct wdc_regs *wdr;
	int i;

	wdr = CHAN_TO_WDC_REGS(&sc->sc_ports[chnum].channel);
	
	/* map the registers */
	wdr->cmd_iot = sc->sc_cmdt;
	bus_space_map(sc->sc_cmdt, chnum * WDC_XSURF_CHANOFF,
	    WDC_XSURF_CHANSIZE, 0, &wdr->cmd_baseioh);

	for (i = 0; i < WDC_NREG; i++)
		bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
		    wdc_xsurf_wdr_offsets[i], i == 0 ? 2 : 1,
		    &wdr->cmd_iohs[i]);
}

int
wdc_xsurf_intr(void *arg)
{
	struct wdc_xsurf_softc *sc;
	uint8_t intreq;
	int r1, r2;

	sc  = (struct wdc_xsurf_softc *)arg;
	r1 = r2 = 0;

	intreq = bus_space_read_1(sc->sc_cmdt, sc->sc_isrh, 0);
	bus_space_write_1(sc->sc_cmdt, sc->sc_isrh, 0, 0); /* pull A11 down */

	/* only one register for both channels... :/ */
	if (intreq & WDC_XSURF_ISR_HIGH) {
		r1 = wdcintr(&sc->sc_ports[0].channel);
		r2 = wdcintr(&sc->sc_ports[1].channel);
	}

	return r1 | r2;
}
