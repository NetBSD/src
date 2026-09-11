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

struct si_payload {
	uint32_t 	insize;		/* bytes to receive. max 128 */
	uint32_t 	outsize;	/* bytes to send. max 128 */
	uint32_t	*status;	/* sisr status for this channel */
	void		*in;		/* buffer to store response */
	void		*out;		/* buffer to send out to device */
};

extern struct cfdriver gcport_cd;

#define SI_SEND		_IOWR(0, 1, struct si_payload)

static int 	gcport_si_match(device_t, cfdata_t, void *);
static void 	gcport_si_attach(device_t, device_t, void *);

static int 	gcport_si_print(void *, const char *);
static int 	siioctl_send(struct si_channel *ch, struct si_payload *sp);
static void 	gcport_si_work(struct work *, void *);

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
	return 1;
}

static void
gcport_si_attach(device_t parent, device_t self, void *aux)
{
	struct si_softc * const psc = device_private(parent);
	struct si_attach_args * const saa = aux;
	struct gcport_softc * const sc = device_private(self);
	struct si_channel *ch = &psc->sc_chan[saa->saa_index];
	int err;

	sc->sc_dev = self;
	sc->ch = ch;
	sc->sc_bst = psc->sc_bst;
	sc->sc_bsh = psc->sc_bsh;
	ch->ch_gcport_dev = self;

	err = workqueue_create(&ch->ch_wqp, "gcport", gcport_si_work, sc,
	    PRI_NONE, IPL_VM, 0);
	if (err != 0) {
		aprint_normal("gcport_si: ch%d failed to create workqueue\n",
		    ch->ch_index);
		ch->ch_wqp = NULL;
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

static void
gcport_si_work(struct work *wk, void *arg)
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
siioctl_send(struct si_channel *ch, struct si_payload *p)
{
	int err, outsize_r, insize_r;
	struct si_packet pk;
	struct sicomcsr_txn txn;

	err = 0;
	txn_init(&txn);
	txn.pk = &pk;

	/* siiobuf must to be written in increments of 4 bytes */
	outsize_r = roundup(p->outsize, 4);
	insize_r = roundup(p->insize, 4);

	pk.out = kmem_zalloc(outsize_r, KM_SLEEP);
	pk.in = kmem_zalloc(insize_r, KM_SLEEP);
	pk.chan = ch->ch_index;
	pk.outsize = p->outsize;
	pk.insize = p->insize;


	if ((err = copyin(p->out, pk.out, pk.outsize)) != 0) {
		goto si_send_cleanup;
	}

	txn.comcsr = (
	    SICOMCSR_CH_EN |
	    SICOMCSR_CMD_EN |
	    SICOMCSR_TCINTMSK |
	    SICOMCSR_RDSTINTMSK |
	    __SHIFTIN(pk.outsize, SICOMCSR_OUTLNGTH) |
	    __SHIFTIN(pk.insize, SICOMCSR_INLNGTH) |
	    __SHIFTIN(pk.chan, SICOMCSR_CHANNEL) |
	    SICOMCSR_TSTART
	);

	if ((err = txn_enqueue(&txn)) != 0) {
		goto si_send_cleanup;
	}

	if ((err = txn_await(&txn)) != 0) {
		err = ETIMEDOUT;
		txn_dequeue(&txn);
		goto si_send_cleanup;
	}

	if ((err = copyout(pk.in, p->in, pk.insize)) != 0) {
		goto si_send_cleanup;
	}

	if ((err = copyout(&pk.status, p->status, sizeof(uint32_t))) != 0) {
		goto si_send_cleanup;
	}
si_send_cleanup:
	txn_destroy(&txn);
	kmem_free(pk.out, outsize_r);
	kmem_free(pk.in, insize_r);
	return err;
}
