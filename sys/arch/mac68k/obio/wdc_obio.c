/*	$NetBSD: wdc_obio.c,v 1.6 2003/09/25 19:29:49 mycroft Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: wdc_obio.c,v 1.6 2003/09/25 19:29:49 mycroft Exp $");

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

#define	WDC_OBIO_REG_NPORTS	(8<<3)
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
	struct  channel_softc *wdc_chanptr;
	struct  channel_softc wdc_channel;
	void    *sc_ih;
};

int	wdc_obio_match	__P((struct device *, struct cfdata *, void *));
void	wdc_obio_attach	__P((struct device *, struct device *, void *));
void	wdc_obio_intr __P((void *));
void	mac68k_bsh_wdc_set_stride __P((bus_space_tag_t t,
	bus_space_handle_t *h, int stride));
u_int16_t	mac68k_bsr2_wdc_gen __P((bus_space_tag_t t,
		bus_space_handle_t *bsh, bus_size_t offset));

CFATTACH_DECL(wdc_obio, sizeof(struct wdc_obio_softc),
    wdc_obio_match, wdc_obio_attach, NULL, NULL);

int
wdc_obio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct obio_attach_args *oa = (struct obio_attach_args *) aux;
	struct channel_softc ch;
	static int wdc_matched = 0;
	int result = 0;

	memset(&ch, 0, sizeof(ch));

	switch (current_mac_model->machineid) {
	case MACH_MACPB150:
	case MACH_MACPB190:
	case MACH_MACPB190CS:
	case MACH_MACP580:
	case MACH_MACQ630:
		ch.cmd_iot = ch.ctl_iot = oa->oa_tag;

		if (bus_space_map(ch.cmd_iot, IDEBase, WDC_OBIO_REG_NPORTS,
				  0, &ch.cmd_ioh))
			return 0;

		mac68k_bsh_wdc_set_stride(ch.cmd_iot, &ch.cmd_ioh, 4);

		if (bus_space_subregion(ch.cmd_iot, ch.cmd_ioh,
				        WDC_OBIO_AUXREG_OFFSET,
					WDC_OBIO_AUXREG_NPORTS, &ch.ctl_ioh))
			return 0;

		result = wdcprobe(&ch);

		bus_space_unmap(ch.cmd_iot, ch.cmd_ioh, WDC_OBIO_REG_NPORTS);

		if (result)
			wdc_matched = 1;
		return (result);
	}
	return 0;
}

static bus_space_tag_t		wdc_obio_isr_tag;
static bus_space_handle_t	wdc_obio_isr_hdl;
static struct channel_softc	*ch_sc = NULL;

void
wdc_obio_intr(arg)
	void *arg;
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

void
wdc_obio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct wdc_obio_softc *sc = (void *)self;
	struct obio_attach_args *oa = aux;
	struct channel_softc *chp = &sc->wdc_channel;

	oa->oa_addr = IDEBase;
	sc->wdc_channel.cmd_iot = sc->wdc_channel.ctl_iot = oa->oa_tag;

	if (bus_space_map(sc->wdc_channel.cmd_iot, oa->oa_addr,
			  WDC_OBIO_REG_NPORTS, 0, &sc->wdc_channel.cmd_ioh)) {
		printf("%s: couldn't map registers\n",
			sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}

	mac68k_bsh_wdc_set_stride(sc->wdc_channel.cmd_iot,
				  &sc->wdc_channel.cmd_ioh, 4);

	if (bus_space_subregion(sc->wdc_channel.cmd_iot,
				sc->wdc_channel.cmd_ioh, WDC_OBIO_AUXREG_OFFSET,
				WDC_OBIO_AUXREG_NPORTS,
				&sc->wdc_channel.ctl_ioh))
		return;

    	wdc_obio_isr_tag = oa->oa_tag;

	if (bus_space_map(wdc_obio_isr_tag,
			  oa->oa_addr+WDC_OBIO_ISR_OFFSET,
			  WDC_OBIO_ISR_NPORTS, 0, &wdc_obio_isr_hdl)) {
		printf("%s: couldn't map intr status register\n",
			sc->sc_wdcdev.sc_dev.dv_xname);
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
	if (sc->sc_wdcdev.sc_dev.dv_cfdata->cf_flags & WDC_CAPABILITY_NOIRQ)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_NOIRQ;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;
	sc->sc_wdcdev.PIO_cap = 0;
	sc->wdc_chanptr = chp;
	sc->sc_wdcdev.channels = &sc->wdc_chanptr;
	sc->sc_wdcdev.nchannels = 1;
	chp->channel = 0;
	chp->wdc = &sc->sc_wdcdev;
	chp->ch_queue = malloc(sizeof(struct channel_queue), M_DEVBUF, M_NOWAIT);

	if (chp->ch_queue == NULL) {
		printf("%s: can't allocate memory for command queue",
			sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}
	
	printf("\n");

	wdcattach(&sc->sc_wdcdev);
}

u_int16_t
mac68k_bsr2_wdc_gen(bus_space_tag_t t, bus_space_handle_t *bsh, bus_size_t offset)
{
	return (*(volatile u_int16_t *) (bsh->base + offset * bsh->stride));
}

void
mac68k_bsh_wdc_set_stride(bus_space_tag_t t, bus_space_handle_t *h, int stride)
{
	h->stride = stride;
	h->bsr1 = mac68k_bsr1_gen;
	h->bsr2 = mac68k_bsr2_wdc_gen;
	h->bsr4 = mac68k_bsr4_gen;
	h->bsrs2 = mac68k_bsrs2_gen;
	h->bsrs4 = mac68k_bsrs4_gen;
	h->bsrm1 = mac68k_bsrm1_gen;
	h->bsrm2 = mac68k_bsrm2_swap;
	h->bsrm4 = mac68k_bsrm4_swap;
	h->bsrms2 = mac68k_bsrm2;
	h->bsrms4 = mac68k_bsrm4;
	h->bsrr1 = mac68k_bsrr1_gen;
	h->bsrr2 = mac68k_bsrr2_gen;
	h->bsrr4 = mac68k_bsrr4_gen;
	h->bsrrs2 = mac68k_bsrrs2_gen;
	h->bsrrs4 = mac68k_bsrrs4_gen;
	h->bsw1 = mac68k_bsw1_gen;
	h->bsw2 = mac68k_bsw2_gen;
	h->bsw4 = mac68k_bsw4_gen;
	h->bsws2 = mac68k_bsws2_gen;
	h->bsws4 = mac68k_bsws4_gen;
	h->bswm2 = mac68k_bswm2_swap;
	h->bswm4 = mac68k_bswm4_swap;
	h->bswms2 = mac68k_bswm2;
	h->bswms4 = mac68k_bswm4;
	h->bswr1 = mac68k_bswr1_gen;
	h->bswr2 = mac68k_bswr2_gen;
	h->bswr4 = mac68k_bswr4_gen;
	h->bswrs2 = mac68k_bswrs2_gen;
	h->bswrs4 = mac68k_bswrs4_gen;
	h->bssm1 = mac68k_bssm1_gen;
	h->bssm2 = mac68k_bssm2_gen;
	h->bssm4 = mac68k_bssm4_gen;
	h->bssr1 = mac68k_bssr1_gen;
	h->bssr2 = mac68k_bssr2_gen;
	h->bssr4 = mac68k_bssr4_gen;
}
