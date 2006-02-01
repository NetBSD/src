/*	$NetBSD: wdc_obio.c,v 1.19.2.1 2006/02/01 14:51:29 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: wdc_obio.c,v 1.19.2.1 2006/02/01 14:51:29 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/callout.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/cpu.h>
#include <machine/viareg.h>

#include <mac68k/obio/obiovar.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#define	WDC_OBIO_REG_NPORTS	0x40
#define	WDC_OBIO_AUXREG_OFFSET	0x38
#define	WDC_OBIO_AUXREG_NPORTS	1
#define	WDC_OBIO_ISR_OFFSET	0x101
#define	WDC_OBIO_ISR_NPORTS	1

static u_long	IDEBase = 0x50f1a000;

/*
 * XXX This code currently doesn't even try to allow 32-bit data port use.
 */

struct wdc_obio_softc {
	struct  wdc_softc sc_wdcdev;
	struct  ata_channel *sc_chanlist[1];
	struct  ata_channel sc_channel;
	struct	ata_queue sc_chqueue;
	struct	wdc_regs sc_wdc_regs;
	void    *sc_ih;
};

int	wdc_obio_match(struct device *, struct cfdata *, void *);
void	wdc_obio_attach(struct device *, struct device *, void *);
void	wdc_obio_intr(void *);

CFATTACH_DECL(wdc_obio, sizeof(struct wdc_obio_softc),
    wdc_obio_match, wdc_obio_attach, NULL, NULL);

static bus_space_tag_t		wdc_obio_isr_tag;
static bus_space_handle_t	wdc_obio_isr_hdl;
static struct ata_channel	*ch_sc;

int
wdc_obio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct obio_attach_args *oa = (struct obio_attach_args *) aux;
	struct ata_channel ch;
	struct wdc_softc wdc;
	struct wdc_regs wdr;
	static int wdc_matched = 0;
	int i, result = 0;

	memset(&wdc, 0, sizeof(wdc));
	memset(&ch, 0, sizeof(ch));
	ch.ch_atac = &wdc.sc_atac;
	wdc.regs = &wdr;

	switch (current_mac_model->machineid) {
	case MACH_MACPB150:
	case MACH_MACPB190:
	case MACH_MACPB190CS:
	case MACH_MACP580:
	case MACH_MACQ630:
		wdr.cmd_iot = wdr.ctl_iot = oa->oa_tag;

		if (bus_space_map(wdr.cmd_iot, IDEBase, WDC_OBIO_REG_NPORTS,
				0, &wdr.cmd_baseioh))
			return 0;

		mac68k_bus_space_handle_swapped(wdr.cmd_iot, &wdr.cmd_baseioh);

		for (i = 0; i < WDC_NREG; i++) {
			if (bus_space_subregion(wdr.cmd_iot, wdr.cmd_baseioh,
					    4 * i, 4, &wdr.cmd_iohs[i]) != 0) {
				return 0;
			}
		}
		wdc_init_shadow_regs(&ch);


		if (bus_space_subregion(wdr.cmd_iot, wdr.cmd_baseioh,
				        WDC_OBIO_AUXREG_OFFSET,
					WDC_OBIO_AUXREG_NPORTS,
					&wdr.ctl_ioh))
			return 0;

		result = wdcprobe(&ch);

		bus_space_unmap(wdr.cmd_iot, wdr.cmd_baseioh, WDC_OBIO_REG_NPORTS);

		if (result)
			wdc_matched = 1;
		return (result);
	}
	return 0;
}

void
wdc_obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct wdc_obio_softc *sc = (void *)self;
	struct wdc_regs *wdr;
	struct obio_attach_args *oa = aux;
	struct ata_channel *chp = &sc->sc_channel;
	int i;

	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;

	oa->oa_addr = IDEBase;
	wdr->cmd_iot = wdr->ctl_iot = oa->oa_tag;

	if (bus_space_map(wdr->cmd_iot, oa->oa_addr,
		      WDC_OBIO_REG_NPORTS, 0, &wdr->cmd_baseioh)) {
		printf("%s: couldn't map registers\n",
			sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
		return;
	}

	mac68k_bus_space_handle_swapped(wdr->cmd_iot,
				  &wdr->cmd_baseioh);

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
				    wdr->cmd_baseioh, 4 * i, 4,
				    &wdr->cmd_iohs[i]) != 0) {
			printf("%s: unable to subregion control register\n",
			    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
			return;
		}
	}
	wdc_init_shadow_regs(&sc->sc_channel);

	if (bus_space_subregion(wdr->cmd_iot,
				wdr->cmd_baseioh,
				WDC_OBIO_AUXREG_OFFSET, WDC_OBIO_AUXREG_NPORTS,
				&wdr->ctl_ioh)) {
		printf("%s: unable to subregion aux register\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
		return;
	}

    	wdc_obio_isr_tag = oa->oa_tag;

	if (bus_space_map(wdc_obio_isr_tag,
			  oa->oa_addr+WDC_OBIO_ISR_OFFSET,
			  WDC_OBIO_ISR_NPORTS, 0, &wdc_obio_isr_hdl)) {
		printf("%s: couldn't map intr status register\n",
			sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
		return;
	}

	switch (current_mac_model->machineid) {
	case MACH_MACP580:
	case MACH_MACQ630:
		/*
		 * Quadra/Performa IDE generates pseudo Nubus intr at slot F
		 */
		printf(" (Quadra/Performa series IDE interface)");

		add_nubus_intr(0xf, (void (*)(void*))wdc_obio_intr, (void *)sc);

		break;
	case MACH_MACPB150:
	case MACH_MACPB190:
	case MACH_MACPB190CS:
		/*
		 * PowerBook IDE generates pseudo NuBus intr at slot C
		 */
		printf(" (PowerBook series IDE interface)");

		add_nubus_intr(0xc, (void (*)(void*))wdc_obio_intr, (void *)sc);

		break;
	}

	ch_sc = chp;
	if (sc->sc_wdcdev.sc_atac.atac_dev.dv_cfdata->cf_flags & ATAC_CAP_NOIRQ)
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_NOIRQ;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_chanlist[0] = chp;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	chp->ch_channel = 0;
	chp->ch_atac = &sc->sc_wdcdev.sc_atac;
	chp->ch_queue = &sc->sc_chqueue;
	chp->ch_ndrive = 2;

	printf("\n");

	wdcattach(chp);
}

void
wdc_obio_intr(void *arg)
{
	unsigned char status;

	status = bus_space_read_1(wdc_obio_isr_tag,
				  wdc_obio_isr_hdl, 0);
	if (status & 0x20) {
		wdcintr(ch_sc);
		bus_space_write_1(wdc_obio_isr_tag,
				  wdc_obio_isr_hdl, 0, status&~0x20);
	}
}
