/* $NetBSD: gcport_si.c,v 1.0 2026/05/18 22:54:30 gummybuns Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gcport_si.c,v 1.0 2026/05/18 22:53:30 gummybuns Exp $");

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

struct si_payload {
	uint32_t 	insize;		/* bytes to receive. max 128 */
	uint32_t 	outsize;	/* bytes to send. max 128 */
	uint32_t	*status;	/* sisr status for this channel */
	void		*in;		/* buffer to store response */
	void		*out;		/* buffer to send out to device */
	long		delay;		/* delay the transaction (microsec) */
};

extern struct cfdriver gcport_cd;
struct gcport_softc {
	device_t		sc_dev;
	struct si_channel	*ch;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};


#define SI_SEND		_IOWR(0, 1, struct si_payload)

static int gcport_si_match(device_t, cfdata_t, void *);
static void gcport_si_attach(device_t, device_t, void *);
static int siioctl_send(struct si_channel *ch, struct si_payload *sp);

dev_type_open(gcport_open);
dev_type_close(gcport_close);
dev_type_ioctl(gcport_ioctl);

const struct cdevsw gcport_cdevsw = {
	.d_open = gcport_open,
	.d_close = gcport_close,
	.d_ioctl = gcport_ioctl,
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

CFATTACH_DECL_NEW(gcport_si, sizeof(struct gcport_softc),
	gcport_si_match, gcport_si_attach, NULL, NULL);

static int
gcport_si_match(device_t parent, cfdata_t cf, void *aux)
{
	struct si_softc * const sc = device_private(parent);
	struct si_attach_args * const saa = aux;
	struct si_channel *ch;
	int unit;
	unsigned chan;

	chan = saa->saa_index;
	ch = &sc->sc_chan[chan];
	unit = cf->cf_unit;

	if (chan == unit && ch->ch_id != 0 && !(IS_GCPAD(ch->ch_id))) {
		aprint_normal("gcport: identified ch%d as a device 0x%08X\n",
		    chan, ch->ch_id);
		return 1;
	}

	return 0;
}

static void
gcport_si_attach(device_t parent, device_t self, void *aux)
{
	struct si_softc * const psc = device_private(parent);
	struct si_attach_args * const saa = aux;
	struct gcport_softc * const sc = device_private(self);
	struct si_channel *ch = &psc->sc_chan[saa->saa_index];

	sc->sc_dev = self;
	sc->ch = ch;
	sc->sc_bst = psc->sc_bst;
	sc->sc_bsh = psc->sc_bsh;
}

int
gcport_open(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct gcport_softc *sc;
	struct si_channel *ch;
	int error;

	sc = device_lookup_private(&gcport_cd, minor(dev));

	if (sc == NULL) {
		return ENXIO;
	}

	ch = sc->ch;
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
gcport_close(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct gcport_softc *sc = device_lookup_private(&gcport_cd, minor(dev));
	struct si_channel *ch = sc->ch;

	mutex_enter(&ch->ch_lock);
	ch->ch_state &= ~(SI_STATE_OPEN | SI_STATE_STOPPED);

	/* cv_init is called in parent's si_attach */
	cv_broadcast(&ch->ch_cv);

	mutex_exit(&ch->ch_lock);
	return 0;
}

int
gcport_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct gcport_softc *sc = device_lookup_private(&gcport_cd, minor(dev));
	struct si_channel *ch = sc->ch;
	int err;

	switch(cmd) {
	case SI_SEND:
		err = siioctl_send(ch, (struct si_payload *)data);
		break;
	default:
		err = EINVAL;
		break;
	}

	return err;
}

static int
siioctl_send(struct si_channel *ch, struct si_payload *gbs)
{
	int err;
	struct si_softc *sc;
	struct siio_send sd;

	err = 0;
	sc = ch->ch_sc;
	if (gbs->outsize > SIIOBUF_SIZE || gbs->insize > SIIOBUF_SIZE) {
		return EINVAL;
	}

	sd.chan = ch->ch_index;
	sd.outsize = gbs->outsize;
	sd.insize = gbs->insize;
	sd.out = kmem_alloc(gbs->outsize, KM_SLEEP);
	sd.in = kmem_alloc(gbs->insize, KM_SLEEP);
	sd.delay = gbs->delay;

	if ((err = copyin(gbs->out, sd.out, sd.outsize)) != 0) {
		goto si_send_cleanup;
	}

	if ((err = __si_send(sc, &sd)) != 0) {
		goto si_send_cleanup;
	}

	if ((err = copyout(sd.in, gbs->in, sd.insize)) != 0) {
		goto si_send_cleanup;
	}

	if ((err = copyout(&sd.status, gbs->status, sizeof(uint32_t))) != 0) {
		goto si_send_cleanup;
	}
si_send_cleanup:
	kmem_free(sd.out, sd.outsize);
	kmem_free(sd.in, sd.insize);
	return err;
}
