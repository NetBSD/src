/*	$NetBSD: wdc_mb.c,v 1.13 2003/09/19 21:35:58 mycroft Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_mb.c,v 1.13 2003/09/19 21:35:58 mycroft Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/bswap.h>
#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/dma.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <m68k/asm_single.h>

#include <atari/dev/ym2149reg.h>
#include <atari/atari/device.h>

/*
 * XXX This code currently doesn't even try to allow 32-bit data port use.
 */
static int	claim_hw __P((void *, int));
static void	free_hw __P((void *));
static void	read_multi_2_swap __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int16_t *, bus_size_t));
static void	write_multi_2_swap __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, const u_int16_t *, bus_size_t));

struct wdc_mb_softc {
	struct wdc_softc sc_wdcdev;
	struct	channel_softc *wdc_chanptr;
	struct  channel_softc wdc_channel;
	void	*sc_ih;
};

int	wdc_mb_probe	__P((struct device *, struct cfdata *, void *));
void	wdc_mb_attach	__P((struct device *, struct device *, void *));

CFATTACH_DECL(wdc_mb, sizeof(struct wdc_mb_softc),
    wdc_mb_probe, wdc_mb_attach, NULL, NULL);

int
wdc_mb_probe(parent, cfp, aux)
	struct device *parent;
	struct cfdata *cfp;
	void *aux;
{
	static int	wdc_matched = 0;
	struct channel_softc ch;
	int	result = 0;
	u_char	sv_ierb;

	if ((machineid & ATARI_TT) || strcmp("wdc", aux) || wdc_matched)
		return 0;
	if (!atari_realconfig)
		return 0;

	memset(&ch, 0, sizeof(ch));

	ch.cmd_iot = ch.ctl_iot = mb_alloc_bus_space_tag();
	if (ch.cmd_iot == NULL)
		return 0;
	ch.cmd_iot->stride = 2;
	ch.cmd_iot->wo_1   = 1;

	if (bus_space_map(ch.cmd_iot, 0xfff00000, 0x40, 0, &ch.cmd_ioh))
		return 0;
	if (bus_space_subregion(ch.cmd_iot, ch.cmd_ioh, 0x38, 1, &ch.ctl_ioh))
		return 0;

	/*
	 * Make sure IDE interrupts are disabled during probing.
	 */
	sv_ierb = MFP->mf_ierb;
	MFP->mf_ierb &= ~IB_DINT;

	/*
	 * Make sure that IDE is turned on on the Falcon.
	 */
	if (machineid & ATARI_FALCON)
		ym2149_ser2(0);

	result = wdcprobe(&ch);

	MFP->mf_ierb = sv_ierb;

	bus_space_unmap(ch.cmd_iot,  ch.cmd_ioh, 0x40);
	mb_free_bus_space_tag(ch.cmd_iot);

	if (result)
		wdc_matched = 1;
	return (result);
}

void
wdc_mb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_mb_softc *sc = (void *)self;

	printf("\n");

	sc->wdc_channel.cmd_iot = sc->wdc_channel.ctl_iot =
	    mb_alloc_bus_space_tag();
	sc->wdc_channel.cmd_iot->stride = 2;
	sc->wdc_channel.cmd_iot->wo_1   = 1;
	sc->wdc_channel.cmd_iot->abs_rms_2 = read_multi_2_swap;
	sc->wdc_channel.cmd_iot->abs_wms_2 = write_multi_2_swap;
	if (bus_space_map(sc->wdc_channel.cmd_iot, 0xfff00000, 0x40, 0,
			  &sc->wdc_channel.cmd_ioh)) {
		printf("%s: couldn't map registers\n",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}
	if (bus_space_subregion(sc->wdc_channel.cmd_iot,
	    sc->wdc_channel.cmd_ioh, 0x38, 1, &sc->wdc_channel.ctl_ioh))
		return;

	/*
	 * Play a nasty trick here. Normally we only manipulate the
	 * interrupt *mask*. However to defeat wd_get_parms(), we
	 * disable the interrupts here using the *enable* register.
	 */
	MFP->mf_ierb &= ~IB_DINT;

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_HWLOCK |
	    WDC_CAPABILITY_ATA_NOSTREAM;
	sc->sc_wdcdev.PIO_cap = 0;
	sc->sc_wdcdev.claim_hw = &claim_hw;
	sc->sc_wdcdev.free_hw  = &free_hw;
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

	/*
	 * Setup & enable disk related interrupts.
	 */
	MFP->mf_ierb |= IB_DINT;
	MFP->mf_iprb  = (u_int8_t)~IB_DINT;
	MFP->mf_imrb |= IB_DINT;

	config_interrupts(self, wdcattach);
}

/*
 * Hardware locking
 */
static int	wd_lock;

static int
claim_hw(softc, maysleep)
void *softc;
int  maysleep;
{
	if (wd_lock != DMA_LOCK_GRANT) {
		if (wd_lock == DMA_LOCK_REQ) {
			/*
			 * ST_DMA access is being claimed.
			 */
			return 0;
		}
		if (!st_dmagrab((dma_farg)wdcintr,
		    (dma_farg)(maysleep ? NULL : wdcrestart), softc,
		    &wd_lock, 1))
			return 0;
	}
	return 1;	
}

static void
free_hw(softc)
void *softc;
{
	/*
	 * Flush pending interrupts before giving-up lock
	 */
	MFP->mf_iprb = (u_int8_t)~IB_DINT;

	/*
	 * Only free the lock on a Falcon. On the Hades, keep it.
	 */
/*	if (machineid & ATARI_FALCON) */
		st_dmafree(softc, &wd_lock);
}

/*
 * XXX
 * This piece of uglyness is caused by the fact that the byte lanes of
 * the data-register are swapped on the atari. This works OK for an IDE
 * disk, but turns into a nightmare when used on atapi devices.
 */
#define calc_addr(base, off, stride, wm)	\
	((u_long)(base) + ((off) << (stride)) + (wm))

static void
read_multi_2_swap(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int16_t		*a;
{
	u_int16_t	*ba;

	ba = (u_int16_t *)calc_addr(h, o, t->stride, t->wo_2);
	for (; c; a++, c--)
		*a = bswap16(*ba);
}

static void
write_multi_2_swap(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int16_t		*a;
{
	u_int16_t	*ba;

	ba = (u_int16_t *)calc_addr(h, o, t->stride, t->wo_2);
	for (; c; a++, c--)
		*ba = bswap16(*a);
}
