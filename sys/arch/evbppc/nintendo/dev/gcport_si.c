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

struct si_control_payload {
	uint32_t	insize;
	uint32_t	outsize;
};

extern struct cfdriver gcport_cd;

#define SI_CTRL_GET	_IOWR(0, 1, struct si_control)
#define SI_CTRL_SET	_IOWR(0, 2, struct si_control_payload)

static void gcport_si_attach(device_t, device_t, void *);
static int gcport_si_match(device_t, cfdata_t, void *);
static int gcport_si_read(dev_t, struct uio *, int);
static int gcport_si_write(dev_t, struct uio *, int);
static int gcport_si_print(void *, const char *);

static void gcport_work(struct work *, void *);

static int siioctl_ctrl_get(struct gcport_softc *, struct si_control *);
static int siioctl_ctrl_set(struct gcport_softc *, struct si_control_payload *);

dev_type_open(gcport_open);
dev_type_close(gcport_close);
dev_type_ioctl(gcport_ioctl);

const struct cdevsw gcport_cdevsw = {
	.d_open = gcport_open,
	.d_close = gcport_close,
	.d_ioctl = gcport_ioctl,
	.d_read = gcport_si_read,
	.d_write = gcport_si_write,
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
	return 1;
}

static void
gcport_si_attach(device_t parent, device_t self, void *aux)
{
	struct si_softc * const psc = device_private(parent);
	struct si_attach_args * const saa = aux;
	struct gcport_softc * const sc = device_private(self);
	struct si_channel *ch = &psc->sc_chan[saa->saa_index];
	struct si_control *ctrl = &sc->ctrl;
	int err;

	sc->sc_dev = self;
	sc->ch = ch;
	sc->sc_bst = psc->sc_bst;
	sc->sc_bsh = psc->sc_bsh;
	ch->ch_gcport_dev = self;
	ch->ch_gcport_sc = sc;

	ctrl->insize = 0;
	ctrl->outsize = 0;
	ctrl->status = SI_AVAILABLE;

	err = workqueue_create(&sc->wqp, "gcport_rdstint",
	    gcport_work, sc, PRI_NONE, IPL_VM, 0);
	if (err != 0) {
		aprint_normal("gcport_si: ch%d failed to create workqueue\n",
		    ch->ch_index);
		sc->wqp = NULL;
	}
}

static int
gcport_si_print(void *aux, const char *pnp)
{
	struct si_attach_args *saa = aux;

	if (pnp != NULL) {
		aprint_normal("uhid at %s", pnp);
	}

	/*
	 * The Wii Operations Manual for RVL-001 refers to the controller
	 * ports as "Nintendo GameCube Controller Sockets".
	 */
	aprint_normal(" socket %d", saa->saa_index + 1);

	return UNCONF;
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
	int err;

	switch(cmd) {
	case SI_CTRL_GET:
		err = siioctl_ctrl_get(sc, (struct si_control *)data);
		break;
	case SI_CTRL_SET:
		err = siioctl_ctrl_set(sc, (struct si_control_payload *)data);
		break;
	default:
		err = EINVAL;
		break;
	}

	return err;
}

static int
gcport_si_read(dev_t dev, struct uio *uio, int ioflag)
{
	struct gcport_softc *sc = device_lookup_private(&gcport_cd, minor(dev));
	struct si_packet *pk = &sc->pk;
	struct si_channel *ch = sc->ch;
	struct si_control *ctrl = &sc->ctrl;
	int err, insize_r, outsize_r;


	if (ctrl->status != SI_COMPLETE) {
		return EBUSY;
	}

	outsize_r = roundup(ctrl->outsize, 4);
	insize_r = roundup(ctrl->insize, 4);

	err = uiomove(pk->in, ctrl->insize, uio);
	kmem_free(pk->out, outsize_r);
	kmem_free(pk->in, insize_r);

	mutex_enter(&ch->ch_lock);
	ctrl->status = SI_AVAILABLE;
	mutex_exit(&ch->ch_lock);

	return err;
}

static int
gcport_si_write(dev_t dev, struct uio *uio, int ioflag)
{
	struct gcport_softc *sc = device_lookup_private(&gcport_cd, minor(dev));
	struct si_packet *pk = &sc->pk;
	struct si_control *ctrl = &sc->ctrl;
	struct si_channel *ch = sc->ch;
	int err, outsize_r, insize_r;

	if (ctrl->status != SI_AVAILABLE) {
		return EBUSY;
	}

	/* siiobuf must to be written in increments of 4 bytes */
	outsize_r = roundup(ctrl->outsize, 4);
	insize_r = roundup(ctrl->insize, 4);

	if (outsize_r > SIIOBUF_SIZE || insize_r > SIIOBUF_SIZE) {
		return EINVAL;
	}

	mutex_enter(&ch->ch_lock);
	pk->out = kmem_zalloc(outsize_r, KM_SLEEP);
	pk->in = kmem_zalloc(insize_r, KM_SLEEP);
	pk->chan = ch->ch_index;
	pk->outsize = ctrl->outsize;
	pk->insize = ctrl->insize;

	err = uiomove(pk->out, pk->outsize, uio);
	mutex_exit(&ch->ch_lock);

	if (err == 0) {
		si_send(ch->ch_sc, pk);
	}

	return err;
}

static void
gcport_work(struct work *wk, void *arg)
{
	struct si_attach_args saa;
	struct gcport_softc *sc;
	struct si_channel *ch;
	int res;

	sc = arg;
	ch = sc->ch;

	if (ch->ch_uhid_dev == NULL) {
		saa.saa_hidev = &ch->ch_hidev;
		saa.saa_index = ch->ch_index;
		ch->ch_uhid_dev = config_found(sc->sc_dev, &saa,
		    gcport_si_print, CFARGS_NONE);
	} else {
		res = config_detach_children(ch->ch_gcport_dev, 0);
		if (res == 0) {
			ch->ch_uhid_dev = NULL;
		}
	}
}

static int
siioctl_ctrl_get(struct gcport_softc *sc, struct si_control *ctrl_out)
{
	struct si_control *ctrl = &sc->ctrl;

	ctrl_out->insize = ctrl->insize;
	ctrl_out->outsize = ctrl->outsize;
	ctrl_out->status = ctrl->status;

	return 0;
}

static int
siioctl_ctrl_set(struct gcport_softc *sc, struct si_control_payload *p)
{
	struct si_control *ctrl = &sc->ctrl;

	if (ctrl->status != SI_AVAILABLE) {
		return EBUSY;
	}

	ctrl->insize = p->insize;
	ctrl->outsize = p->outsize;

	return 0;
}
