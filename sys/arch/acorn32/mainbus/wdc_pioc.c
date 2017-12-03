/*	$NetBSD: wdc_pioc.c,v 1.28.2.1 2017/12/03 11:35:45 jdolecek Exp $	*/

/*
 * Copyright (c) 1997-1998 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_pioc.c,v 1.28.2.1 2017/12/03 11:35:45 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/bus.h>

#include <machine/intr.h>

#include <acorn32/mainbus/piocvar.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

#include "locators.h"

#define WDC_PIOC_REG_NPORTS	8
#define WDC_PIOC_AUXREG_OFFSET	(0x206 * 4)
#define WDC_PIOC_AUXREG_NPORTS	1

struct wdc_pioc_softc {
	struct	wdc_softc sc_wdcdev;
	struct	ata_channel *sc_chanlist[1];
	struct	ata_channel sc_channel;
	struct	wdc_regs sc_wdc_regs;
	void	*sc_ih;
};

/* prototypes for functions */
static int  wdc_pioc_probe  (device_t, cfdata_t, void *);
static void wdc_pioc_attach (device_t, device_t, void *);

/* device attach structure */
CFATTACH_DECL_NEW(wdc_pioc, sizeof(struct wdc_pioc_softc),
    wdc_pioc_probe, wdc_pioc_attach, NULL, NULL);

/*
 * int wdc_pioc_probe(device_t parent, cfdata_t cf, void *aux)
 *
 * Make sure we are trying to attach a wdc device and then
 * probe for one.
 */

static int
wdc_pioc_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct pioc_attach_args *pa = aux;
	struct wdc_regs wdr;
	int res, i;
	u_int iobase;

	if (pa->pa_name && strcmp(pa->pa_name, "wdc") != 0)
		return(0);

	/* We need an offset */
	if (pa->pa_offset == PIOCCF_OFFSET_DEFAULT)
		return(0);

	iobase = pa->pa_iobase + pa->pa_offset;
	wdr.cmd_iot = pa->pa_iot;
	wdr.ctl_iot = pa->pa_iot;

	if (bus_space_map(wdr.cmd_iot, iobase, WDC_PIOC_REG_NPORTS, 0,
	    &wdr.cmd_baseioh))
		return(0);
	for (i = 0; i < WDC_PIOC_REG_NPORTS; i++) {
		if (bus_space_subregion(wdr.cmd_iot, wdr.cmd_baseioh, i,
			i == 0 ? 4 : 1, &wdr.cmd_iohs[i]) != 0) {
			bus_space_unmap(wdr.cmd_iot, wdr.cmd_baseioh,
			    WDC_PIOC_REG_NPORTS);
			return 0;
		}
	}
	wdc_init_shadow_regs(&wdr);

	if (bus_space_map(wdr.ctl_iot, iobase + WDC_PIOC_AUXREG_OFFSET,
	    WDC_PIOC_AUXREG_NPORTS, 0, &wdr.ctl_ioh)) {
		bus_space_unmap(wdr.cmd_iot, wdr.cmd_baseioh,
		    WDC_PIOC_REG_NPORTS);
		return(0);
	}

	res = wdcprobe(&wdr);

	bus_space_unmap(wdr.ctl_iot, wdr.ctl_ioh, WDC_PIOC_AUXREG_NPORTS);
	bus_space_unmap(wdr.cmd_iot, wdr.cmd_baseioh, WDC_PIOC_REG_NPORTS);

	if (res)
		 pa->pa_iosize = WDC_PIOC_REG_NPORTS;
	return(res);
}

/*
 * void wdc_pioc_attach(device_t parent, device_t self, void *aux)
 *
 * attach the wdc device
 */

static void
wdc_pioc_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_pioc_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	struct pioc_attach_args *pa = aux;
	u_int iobase;
	int i;

	aprint_normal("\n");

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;
	iobase = pa->pa_iobase + pa->pa_offset;
	wdr->cmd_iot = pa->pa_iot;
	wdr->ctl_iot = pa->pa_iot;
	if (bus_space_map(wdr->cmd_iot, iobase,
	    WDC_PIOC_REG_NPORTS, 0, &wdr->cmd_baseioh))
		panic("%s: couldn't map drive registers", device_xname(self));
	for (i = 0; i < WDC_PIOC_REG_NPORTS; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
			wdr->cmd_baseioh, i,	i == 0 ? 4 : 1,
			&wdr->cmd_iohs[i]) != 0)
			panic("%s: couldn't submap drive registers",
			    device_xname(self));
	}

	if (bus_space_map(wdr->ctl_iot,
	    iobase + WDC_PIOC_AUXREG_OFFSET, WDC_PIOC_AUXREG_NPORTS, 0,
	    &wdr->ctl_ioh))
		panic("%s: couldn't map aux registers", device_xname(self));

	sc->sc_ih = intr_claim(pa->pa_irq, IPL_BIO, "wdc",  wdcintr,
	     &sc->sc_channel);
	if (!sc->sc_ih)
		panic("%s: Cannot claim IRQ %d", device_xname(self),
		    pa->pa_irq);
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_chanlist[0] = &sc->sc_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_channel.ch_atac = &sc->sc_wdcdev.sc_atac;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->sc_channel.ch_channel = 0;

	wdc_init_shadow_regs(wdr);

	wdcattach(&sc->sc_channel);
}

/* End of wdc_pioc.c */
