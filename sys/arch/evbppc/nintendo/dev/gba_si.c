/*-
 * Copyright (c) 2025 ZacBrown <gummybuns@protonmail.com>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gba_si.c,v 1.0 2026/04/24 15:07:30 gummybuns Exp $");

#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>
#include <dev/hid/uhid.h>

#include "si.h"
#include "joybus.h"

static int gba_si_match(device_t, cfdata_t, void *);
static void gba_si_attach(device_t, device_t, void *);

dev_type_open(gba_open);
dev_type_close(gba_close);
dev_type_ioctl(gba_ioctl);

const struct cdevsw gba_cdevsw = {
	.d_open = gba_open,
	.d_close = gba_close,
	.d_ioctl = noioctl,
	.d_read = noread,
	.d_write = nowrite,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

struct gba_softc {
	device_t		sc_dev;
	struct si_channel	*ch;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

CFATTACH_DECL_NEW(gba_si, sizeof(struct gba_softc),
	gba_si_match, gba_si_attach, NULL, NULL);

static int
gba_si_match(device_t parent, cfdata_t cf, void *aux)
{
	struct si_softc * const sc = device_private(parent);
	struct si_attach_args * const saa = aux;
	struct si_channel *ch;

	ch = &sc->sc_chan[saa->saa_index];
	aprint_normal("gba: checking 0x%08X...", ch->ch_id);
	if (IS_GBA(ch->ch_id)) {
		aprint_normal(" is is a gba!\n");
		return 1;
	}

	aprint_normal(" is not a gba\n");
	return 0;
}

static void
gba_si_attach(device_t parent, device_t self, void *aux)
{
	struct si_softc * const psc = device_private(parent);
	struct si_attach_args * const saa = aux;
	struct gba_softc * const sc = device_private(self);
	struct si_channel *ch = &psc->sc_chan[saa->saa_index];

	aprint_normal("gba: inside attach\n");
	sc->sc_dev = self;
	sc->ch = ch;
	sc->sc_bst = psc->sc_bst;
	sc->sc_bsh = psc->sc_bsh;
}

int
gba_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	return 0;
}

int
gba_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	return 0;
}

/*
int
gba_read(dev_t dev, struct uio *uio, int flags)
{
	return 0;
}
*/
