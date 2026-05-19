/* $NetBSD: gba_si.c,v 1.0 2026/05/18 22:54:30 gummybuns Exp $ */

/*-
 * Copyright (c) 2026 ZacBrown <gummybuns@protonmail.com>
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
__KERNEL_RCSID(0, "$NetBSD: gba_si.c,v 1.0 2026/05/18 22:53:30 gummybuns Exp $");

#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioccom.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/types.h>

#include <dev/usb/usb.h>
#include <dev/hid/uhid.h>

#include "si.h"
#include "joybus.h"
#include "gba_multiboot.h"

struct gba_send {
	uint32_t	status;		/* sisr status for this channel */
	uint32_t 	insize;		/* bytes to receive. max 128 */
	uint32_t 	outsize;	/* bytes to send. max 128 */
	void		*in;		/* buffer to store response */
	void		*out;		/* buffer to send out to gba */
};

struct gba_multiboot {
	long	size;
	void	*rom;
};

extern struct cfdriver gba_cd;
struct gba_softc {
	device_t		sc_dev;
	struct si_channel	*ch;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};


#define GBA_SEND     	_IOWR(0, 1, struct gba_send)
#define GBA_MULTIBOOT	_IOWR(0, 2, struct gba_multiboot)

static int gba_si_match(device_t, cfdata_t, void *);
static void gba_si_attach(device_t, device_t, void *);

dev_type_open(gba_open);
dev_type_close(gba_close);
dev_type_ioctl(gba_ioctl);

const struct cdevsw gba_cdevsw = {
	.d_open = gba_open,
	.d_close = gba_close,
	.d_ioctl = gba_ioctl,
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

CFATTACH_DECL_NEW(gba_si, sizeof(struct gba_softc),
	gba_si_match, gba_si_attach, NULL, NULL);

static int
gba_si_match(device_t parent, cfdata_t cf, void *aux)
{
	struct si_softc * const sc = device_private(parent);
	struct si_attach_args * const saa = aux;
	struct si_channel *ch;
	unsigned chan;

	chan = saa->saa_index;
	ch = &sc->sc_chan[chan];
	aprint_normal("gba: checking 0x%08X...\n", ch->ch_id);
	if (ch->ch_id == JB_GBA) {
		aprint_normal("gba: identified ch%d as a gba device\n", chan);
		return 1;
	}

	return 0;
}

static void
gba_si_attach(device_t parent, device_t self, void *aux)
{
	struct si_softc * const psc = device_private(parent);
	struct si_attach_args * const saa = aux;
	struct gba_softc * const sc = device_private(self);
	struct si_channel *ch = &psc->sc_chan[saa->saa_index];

	sc->sc_dev = self;
	sc->ch = ch;
	sc->sc_bst = psc->sc_bst;
	sc->sc_bsh = psc->sc_bsh;
	// TODO - i dont know if i need to disable polling or not
	//WR4(psc, SIPOLL, 0);
}

int
gba_open(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct gba_softc * const sc = device_lookup_private(&gba_cd, minor(dev));
	struct si_channel *ch = sc->ch;
	int error;

	mutex_enter(&ch->ch_lock);

	if (ISSET(ch->ch_state, SI_STATE_OPEN)) {
		error = EBUSY;
		goto unlock;
	}

	ch->ch_state |= SI_STATE_OPEN;
	error = 0;
unlock:
	mutex_exit(&ch->ch_lock);
	return error;
}

int
gba_close(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct gba_softc * const sc = device_lookup_private(&gba_cd,
	    minor(dev));
	struct si_channel *ch = sc->ch;

	mutex_enter(&ch->ch_lock);
	ch->ch_state &= ~(SI_STATE_OPEN | SI_STATE_STOPPED);

	/* cv_init is called in parent's si_attach */
	cv_broadcast(&ch->ch_cv);

	mutex_exit(&ch->ch_lock);
	return 0;
}

/**
 * TODO:
 *
 * i dont really want to touch the other guys code at all if i dont have to.
 * i dont think that i do. according to the docs you should never set
 * TSTART if you are in the middle of a transaction. i can add the AWAIT to
 * one or two of his lines np.
 *
 * then it says you should never change OUTLENGTH / INLENGTH / CHANNEL in the
 * middle of a transaction. these are only things used for the siiobuf, which
 * no one is using but me. so i can define my own mutex for that purpose.
 */
int
gba_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct gba_softc * const gbasc = device_lookup_private(&gba_cd,
	    minor(dev));
	struct si_channel *ch = gbasc->ch;
	struct si_softc * const sc = ch->ch_sc;
	int err;

	switch(cmd) {
	case GBA_MULTIBOOT:
		err = 0;
		struct gba_multiboot *gbm = (struct gba_multiboot *)data;
		struct gba_multiboot_p gbmp;
		void *rom = kmem_alloc(gbm->size, KM_SLEEP);
		if ((err = copyin(gbm->rom, rom, gbm->size)) != 0) {
			goto multiboot_cleanup;
		}

		gbmp.sc = sc;
		gbmp.chan = ch->ch_index;
		gbmp.rom = (unsigned char *)rom;
		gbmp.size = gbm->size;

		err = __gba_multiboot(&gbmp);
multiboot_cleanup:
		kmem_free(rom, gbm->size);
		break;
	case GBA_SEND:
		err = 0;
		struct siio_send sd;
		struct gba_send *gbs = (struct gba_send *)data;
		if (gbs->outsize > SIIOBUF_SIZE || gbs->insize > SIIOBUF_SIZE) {
			return EINVAL;
		}

		sd.chan = ch->ch_index;
		sd.outsize = gbs->outsize;
		sd.insize = gbs->insize;
		sd.out = kmem_alloc(gbs->outsize, KM_SLEEP);
		sd.in = kmem_alloc(gbs->insize, KM_SLEEP);

		if ((err = copyin(gbs->out, sd.out, sd.outsize)) != 0) {
			goto si_send_cleanup;
		}

		if ((err = __si_send(sc, &sd)) != 0) {
			goto si_send_cleanup;
		}

		if ((err = copyout(sd.in, gbs->in, sd.insize)) != 0) {
			goto si_send_cleanup;
		}

		gbs->status = sd.status;
si_send_cleanup:
		kmem_free(sd.out, sd.outsize);
		kmem_free(sd.in, sd.insize);
		break;
	default:
		err = EINVAL;
		break;
	}

	return err;
}
