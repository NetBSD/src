/*	$NetBSD: wdc_isa.c,v 1.1.4.2 2001/04/21 17:53:24 bouyer Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#define	WDC_ISA_REG_NPORTS	8
#define	WDC_ISA_AUXREG_OFFSET	0x206
#define	WDC_ISA_AUXREG_NPORTS	1 /* XXX "fdc" owns ports 0x3f7/0x377 */

/* options passed via the 'flags' config keyword */
#define WDC_OPTIONS_32	0x01 /* try to use 32bit data I/O */

struct wdc_isa_softc {
	struct	wdc_softc sc_wdcdev;
	struct	channel_softc *wdc_chanptr;
	struct	channel_softc wdc_channel;
	isa_chipset_tag_t sc_ic;
	void	*sc_ih;
	int	sc_drq;
};

static void	wdc_read_multi_2_swap __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int16_t *, bus_size_t));
static void	wdc_write_multi_2_swap __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, const u_int16_t *, bus_size_t));
static void	wdc_read_multi_4_swap __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int32_t *, bus_size_t));
static void	wdc_write_multi_4_swap __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, const u_int32_t *, bus_size_t));

int	wdc_isa_probe	__P((struct device *, struct cfdata *, void *));
void	wdc_isa_attach	__P((struct device *, struct device *, void *));

struct cfattach wdcisa_ca = {
	sizeof(struct wdc_isa_softc), wdc_isa_probe, wdc_isa_attach
};

int
wdc_isa_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct channel_softc ch;
	struct isa_attach_args *ia = aux;
	int result = 0;

	memset(&ch, 0, sizeof(ch));

	ch.cmd_iot = ia->ia_iot;
	if (bus_space_map(ch.cmd_iot, ia->ia_iobase, WDC_ISA_REG_NPORTS, 0,
	    &ch.cmd_ioh))
		goto out;

	ch.ctl_iot = ia->ia_iot;
	if (bus_space_map(ch.ctl_iot, ia->ia_iobase + WDC_ISA_AUXREG_OFFSET,
	    WDC_ISA_AUXREG_NPORTS, 0, &ch.ctl_ioh))
		goto outunmap;

	result = wdcprobe(&ch);
	if (result) {
		ia->ia_iosize = WDC_ISA_REG_NPORTS;
		ia->ia_msize = 0;
	}

	bus_space_unmap(ch.ctl_iot, ch.ctl_ioh, WDC_ISA_AUXREG_NPORTS);
outunmap:
	bus_space_unmap(ch.cmd_iot, ch.cmd_ioh, WDC_ISA_REG_NPORTS);
out:
	return (result);
}

void
wdc_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_isa_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;

	printf("\n");

	/*
	 * The differences with the version in .../dev/isa/wdc_isa.c are
 	 * just below this point. The Milan need a byte-swap of the data.
	 * We do this by making a local copy of the isa_iot and shove in
	 * the different streaming methods.
	 */
	sc->wdc_channel.cmd_iot = malloc(sizeof(*sc->wdc_channel.cmd_iot),
	    				M_DEVBUF, M_NOWAIT);
	if (sc->wdc_channel.cmd_iot == NULL) {
		printf("%s: can't allocate memory for io-tag",
			sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}
	sc->wdc_channel.ctl_iot = sc->wdc_channel.cmd_iot;
	*sc->wdc_channel.cmd_iot = *ia->ia_iot;
	sc->wdc_channel.cmd_iot->abs_rms_2 = wdc_read_multi_2_swap;
	sc->wdc_channel.cmd_iot->abs_wms_2 = wdc_write_multi_2_swap;
	sc->wdc_channel.cmd_iot->abs_rms_4 = wdc_read_multi_4_swap;
	sc->wdc_channel.cmd_iot->abs_wms_4 = wdc_write_multi_4_swap;

	/* Resume normal attach cruft */
	sc->sc_ic = ia->ia_ic;
	if (bus_space_map(sc->wdc_channel.cmd_iot, ia->ia_iobase,
	    WDC_ISA_REG_NPORTS, 0, &sc->wdc_channel.cmd_ioh) ||
	    bus_space_map(sc->wdc_channel.ctl_iot,
	      ia->ia_iobase + WDC_ISA_AUXREG_OFFSET, WDC_ISA_AUXREG_NPORTS,
	      0, &sc->wdc_channel.ctl_ioh)) {
		printf("%s: couldn't map registers\n",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}
	sc->wdc_channel.data32iot = sc->wdc_channel.cmd_iot;
	sc->wdc_channel.data32ioh = sc->wdc_channel.cmd_ioh;

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_BIO, wdcintr, &sc->wdc_channel);

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_PREATA;
	if (sc->sc_wdcdev.sc_dev.dv_cfdata->cf_flags & WDC_OPTIONS_32)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA32;
	sc->sc_wdcdev.PIO_cap = 0;
	sc->wdc_chanptr = &sc->wdc_channel;
	sc->sc_wdcdev.channels = &sc->wdc_chanptr;
	sc->sc_wdcdev.nchannels = 1;
	sc->wdc_channel.channel = 0;
	sc->wdc_channel.wdc = &sc->sc_wdcdev;
	sc->wdc_channel.ch_queue = malloc(sizeof(struct channel_queue),
	    M_DEVBUF, M_NOWAIT);
	if (sc->wdc_channel.ch_queue == NULL) {
		printf("%s: can't allocate memory for command queue",
		sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}
	wdcattach(&sc->wdc_channel);
}

static void
wdc_read_multi_2_swap(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int16_t		*a;
{
	u_int16_t	*ba;

	ba = (u_int16_t *)(h + o);
	for (; c; a++, c--)
		*a = bswap16(*ba);
}

static void
wdc_write_multi_2_swap(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int16_t		*a;
{
	u_int16_t	*ba;

	ba = (u_int16_t *)(h + o);
	for (; c; a++, c--)
		*ba = bswap16(*a);
}

static void
wdc_read_multi_4_swap(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int32_t		*a;
{
	u_int32_t	*ba;

	ba = (u_int32_t *)(h + o);
	for (; c; a++, c--)
		*a = bswap32(*ba);
}

static void
wdc_write_multi_4_swap(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int32_t		*a;
{
	u_int32_t	*ba;

	ba = (u_int32_t *)(h + o);
	for (; c; a++, c--)
		*ba = bswap32(*a);
}
