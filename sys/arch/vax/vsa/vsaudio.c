/*	$NetBSD: vsaudio.c,v 1.7 2020/09/12 05:19:16 isaki Exp $	*/
/*	$OpenBSD: vsaudio.c,v 1.4 2013/05/15 21:21:11 ratchov Exp $	*/

/*
 * Copyright (c) 2011 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 1995 Rolf Grossmann
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * Audio backend for the VAXstation 4000 AMD79C30 audio chip.
 * Currently working in pseudo-DMA mode; DMA operation may be possible and
 * needs to be investigated.
 */
/*
 * Although he did not claim copyright for his work, this code owes a lot
 * to Blaz Antonic <blaz.antonic@siol.net> who figured out a working
 * interrupt triggering routine in vsaudio_match().
 */
/*
 * Ported to NetBSD, from OpenBSD, by Björn Johannesson (rherdware@yahoo.com)
 * in December 2014
 */

#include "audio.h"
#if NAUDIO > 0

#include <sys/errno.h>
#include <sys/evcnt.h>
#include <sys/intr.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/vsbus.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <dev/ic/am7930reg.h>
#include <dev/ic/am7930var.h>

/* physical addresses of the AM79C30 chip */
#define VSAUDIO_CSR			0x200d0000
#define VSAUDIO_CSR_KA49		0x26800000

struct vsaudio_softc {
	struct am7930_softc sc_am7930;	/* glue to MI code */

	bus_space_tag_t sc_bt;		/* bus cookie */
	bus_space_handle_t sc_bh;	/* device registers */
};

static int vsaudio_match(struct device *parent, struct cfdata *match, void *);
static void vsaudio_attach(device_t parent, device_t self, void *);

static void vsaudio_hwintr(void *);

CFATTACH_DECL_NEW(vsaudio, sizeof(struct vsaudio_softc), vsaudio_match,
		vsaudio_attach, NULL, NULL);

/*
 * Hardware access routines for the MI code
 */
uint8_t	vsaudio_codec_dread(struct am7930_softc *, int);
void	vsaudio_codec_dwrite(struct am7930_softc *, int, uint8_t);

struct am7930_glue vsaudio_glue = {
	vsaudio_codec_dread,
	vsaudio_codec_dwrite,
};

/*
 * Interface to the MI audio layer.
 */
int	vsaudio_getdev(void *, struct audio_device *);

struct audio_hw_if vsaudio_hw_if = {
	.query_format		= am7930_query_format,
	.set_format		= am7930_set_format,
	.commit_settings	= am7930_commit_settings,
	.trigger_output		= am7930_trigger_output,
	.trigger_input		= am7930_trigger_input,
	.halt_output		= am7930_halt_output,
	.halt_input		= am7930_halt_input,
	.getdev			= vsaudio_getdev,
	.set_port		= am7930_set_port,
	.get_port		= am7930_get_port,
	.query_devinfo		= am7930_query_devinfo,
	.get_props		= am7930_get_props,
	.get_locks		= am7930_get_locks,
};


struct audio_device vsaudio_device = {
	"am7930",
	"x",
	"vsaudio"
};


static int
vsaudio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct vsbus_attach_args *va = aux;
	volatile uint32_t *regs;
	int i;

	switch (vax_boardtype) {
#if defined(VAX_BTYP_46) || defined(VAX_BTYP_48)
	case VAX_BTYP_46:
	case VAX_BTYP_48:
		if (va->va_paddr != VSAUDIO_CSR)
			return 0;
		break;
#endif
#if defined(VAX_BTYP_49)
	case VAX_BTYP_49:
		if (va->va_paddr != VSAUDIO_CSR_KA49)
			return 0;
		break;
#endif
	default:
		return 0;
	}

	regs = (volatile uint32_t *)va->va_addr;
	regs[AM7930_DREG_CR] = AM7930_IREG_INIT;
	regs[AM7930_DREG_DR] = AM7930_INIT_PMS_ACTIVE | AM7930_INIT_INT_ENABLE;

	regs[AM7930_DREG_CR] = AM7930_IREG_MUX_MCR1;
	regs[AM7930_DREG_DR] = 0;

	regs[AM7930_DREG_CR] = AM7930_IREG_MUX_MCR2;
	regs[AM7930_DREG_DR] = 0;

	regs[AM7930_DREG_CR] = AM7930_IREG_MUX_MCR3;
	regs[AM7930_DREG_DR] = (AM7930_MCRCHAN_BB << 4) | AM7930_MCRCHAN_BA;

	regs[AM7930_DREG_CR] = AM7930_IREG_MUX_MCR4;
	regs[AM7930_DREG_DR] = AM7930_MCR4_INT_ENABLE;

	for (i = 10; i < 20; i++)
		regs[AM7930_DREG_BBTB] = i;
	delay(1000000); /* XXX too large */

	return 1;
}


static void
vsaudio_attach(device_t parent, device_t self, void *aux)
{
	struct vsbus_attach_args *va = aux;
	struct vsaudio_softc *sc = device_private(self);
	struct am7930_softc *amsc = &sc->sc_am7930;

	if (bus_space_map(va->va_memt, va->va_paddr, AM7930_DREG_SIZE << 2, 0,
	    &sc->sc_bh) != 0) {
		printf(": can't map registers\n");
		return;
	}
	sc->sc_bt = va->va_memt;
	amsc->sc_dev = self;
	amsc->sc_glue = &vsaudio_glue;
	am7930_init(amsc, AUDIOAMD_POLL_MODE);
	scb_vecalloc(va->va_cvec, vsaudio_hwintr, amsc, SCB_ISTACK,
	    &amsc->sc_intrcnt);
	evcnt_attach_dynamic(&amsc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "intr");

	aprint_normal("\n");
	audio_attach_mi(&vsaudio_hw_if, sc, self);
}

static void
vsaudio_hwintr(void *arg)
{

	am7930_hwintr(arg);
}

/* direct read */
uint8_t
vsaudio_codec_dread(struct am7930_softc *amsc, int reg)
{
	struct vsaudio_softc *sc = (struct vsaudio_softc *)amsc;

	return bus_space_read_1(sc->sc_bt, sc->sc_bh, reg << 2);
}

/* direct write */
void
vsaudio_codec_dwrite(struct am7930_softc *amsc, int reg, uint8_t val)
{
	struct vsaudio_softc *sc = (struct vsaudio_softc *)amsc;

	bus_space_write_1(sc->sc_bt, sc->sc_bh, reg << 2, val);
}

int
vsaudio_getdev(void *addr, struct audio_device *retp)
{

	*retp = vsaudio_device;
	return 0;
}

#endif /* NAUDIO > 0 */
