/*	$NetBSD: bthci.c,v 1.14 2003/07/24 19:19:42 nathanw Exp $	*/

/*
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) and
 * David Sainty (David.Sainty@dtsp.co.nz).
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
__KERNEL_RCSID(0, "$NetBSD: bthci.c,v 1.14 2003/07/24 19:19:42 nathanw Exp $");

#include "bthcidrv.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/vnode.h>

#include <dev/bluetooth/bluetooth.h>
#include <dev/bluetooth/bthci_util.h>
#include <dev/bluetooth/bthcivar.h>

#ifdef BTHCI_DEBUG
#define DPRINTF(x)	if (bthcidebug) printf x
#define DPRINTFN(n,x)	if (bthcidebug>(n)) printf x
#define Static
int bthcidebug = 99;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#define Static static
#endif

struct bthci_softc {
	struct	device			sc_dev;
	struct btframe_methods const	*sc_methods;
	void				*sc_handle;

	u_int8_t			*sc_rd_buf;
	size_t				sc_rd_len;
	struct selinfo			sc_rd_sel;

	int				sc_refcnt;
	int				sc_outputready;
	char				sc_open;
	char				sc_dying;
};

int bthci_match(struct device *parent, struct cfdata *match, void *aux);
void bthci_attach(struct device *parent, struct device *self, void *aux);
int bthci_activate(struct device *self, enum devact act);
int bthci_detach(struct device *self, int flags);

#if NBTHCIDRV == 0
/* In case we just have tty attachment. */
struct cfdriver bthci_cd = {
	NULL, "bthci", DV_DULL
};
#endif

CFATTACH_DECL(bthci, sizeof(struct bthci_softc),
    bthci_match, bthci_attach, bthci_detach, bthci_activate);

extern struct cfdriver bthci_cd;

dev_type_open(bthciopen);
dev_type_close(bthciclose);
dev_type_poll(bthcipoll);
dev_type_kqfilter(bthcikqfilter);

static int bthciread(dev_t dev, struct uio *uio, int flag);
static int bthciwrite(dev_t dev, struct uio *uio, int flag);

static void bthci_recveventdata(void *h, u_int8_t *data, size_t len);
static void bthci_recvacldata(void *h, u_int8_t *data, size_t len);
static void bthci_recvscodata(void *h, u_int8_t *data, size_t len);

const struct cdevsw bthci_cdevsw = {
	bthciopen, bthciclose, bthciread, bthciwrite, noioctl,
	nostop, notty, bthcipoll, nommap, bthcikqfilter,
};

#define BTHCIUNIT(dev) (minor(dev))

struct btframe_callback_methods const bthci_callbacks = {
	bthci_recveventdata, bthci_recvacldata, bthci_recvscodata
};

int
bthci_match(struct device *parent, struct cfdata *match, void *aux)
{
	/*struct bt_attach_args *bt = aux;*/

	return (1);
}

void
bthci_attach(struct device *parent, struct device *self, void *aux)
{
	struct bthci_softc *sc = (struct bthci_softc *)self;
	struct bt_attach_args *bt = aux;

	sc->sc_methods = bt->bt_methods;
	sc->sc_handle = bt->bt_handle;

	*bt->bt_cb = &bthci_callbacks;

#ifdef DIAGNOSTIC
	if (sc->sc_methods->bt_open == NULL ||
	    sc->sc_methods->bt_close == NULL ||

	    sc->sc_methods->bt_control.bt_alloc == NULL ||
	    sc->sc_methods->bt_control.bt_send == NULL ||

	    sc->sc_methods->bt_acldata.bt_alloc == NULL ||
	    sc->sc_methods->bt_acldata.bt_send == NULL ||

	    sc->sc_methods->bt_scodata.bt_alloc == NULL ||
	    sc->sc_methods->bt_scodata.bt_send == NULL)
		panic("%s: missing methods", sc->sc_dev.dv_xname);
#endif

	printf("\n");
}

int
bthci_activate(struct device *self, enum devact act)
{
	/*struct bthci_softc *sc = (struct bthci_softc *)self;*/

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		break;
	}
	return (0);
}

static void
bthci_abortdealloc(struct bthci_softc *sc)
{
	DPRINTFN(0, ("%s: sc=%p\n", __func__, sc));

	if (sc->sc_rd_buf != NULL) {
		free(sc->sc_rd_buf, M_TEMP);
		sc->sc_rd_buf = NULL;
	}
}

int
bthci_detach(struct device *self, int flags)
{
	struct bthci_softc *sc = (struct bthci_softc *)self;
	int maj, mn;

	sc->sc_dying = 1;
	wakeup(&sc->sc_outputready);

	DPRINTFN(1, ("%s: waiting for refcount (%d)\n",
		     __func__, sc->sc_refcnt));

	if (--sc->sc_refcnt >= 0)
		tsleep(&sc->sc_refcnt, PZERO, "bthcdt", 0);

	DPRINTFN(1, ("%s: refcount complete\n", __func__));

	bthci_abortdealloc(sc);

	/* locate the major number */
	maj = cdevsw_lookup_major(&bthci_cdevsw);

	/* Nuke the vnodes for any open instances (calls close). */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);

	DPRINTFN(1, ("%s: driver detached\n", __func__));

	return (0);
}

int
bthciopen(dev_t dev, int flag, int mode, struct proc *p)
{
	struct bthci_softc *sc;
	int error;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (EIO);
	if (sc->sc_open)
		return (EBUSY);

	sc->sc_rd_buf = malloc(BTHCI_ACL_DATA_MAX_LEN + 1, M_TEMP, M_NOWAIT);
	if (sc->sc_rd_buf == NULL)
		return (ENOMEM);

	sc->sc_refcnt++;

	if (sc->sc_methods->bt_open != NULL) {
		error = sc->sc_methods->bt_open(sc->sc_handle, flag, mode, p);
		if (error)
			goto bad;
	}

	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);

	sc->sc_open = 1;
	return (0);

 bad:
	free(sc->sc_rd_buf, M_TEMP);
	sc->sc_rd_buf = NULL;

	return error;
}

int
bthciclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct bthci_softc *sc;
	int error;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);

	if (!sc->sc_open)
		return (0);

	sc->sc_refcnt++;

	sc->sc_open = 0;

	if (sc->sc_methods->bt_close != NULL)
		error = sc->sc_methods->bt_close(sc->sc_handle, flag, mode, p);
	else
		error = 0;

	bthci_abortdealloc(sc);

	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);

	return (error);
}

static int
bthciread(dev_t dev, struct uio *uio, int flag)
{
	struct bthci_softc *sc;
	int error;
	int s;
	unsigned int blocked;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);

	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 ||
	    !sc->sc_open || sc->sc_dying)
		return (EIO);

	DPRINTFN(1, ("%s: length=%lu\n", __func__,
		     (unsigned long)uio->uio_resid));

	sc->sc_refcnt++;

	s = sc->sc_methods->bt_splraise();
	while (!sc->sc_outputready) {
#define BTHCI_READ_EVTS (BT_CBBLOCK_ACL_DATA | BT_CBBLOCK_EVENT)
		blocked = sc->sc_methods->bt_unblockcb(sc->sc_handle,
						       BTHCI_READ_EVTS);
		if ((blocked & BTHCI_READ_EVTS) != 0) {
			/* Problem unblocking events */
			splx(s);
			DPRINTFN(1,("%s: couldn't unblock events\n",
				    __func__));
			error = EIO;
			goto ret;
		}

		DPRINTFN(5,("%s: calling tsleep()\n", __func__));
		error = tsleep(&sc->sc_outputready, PZERO | PCATCH,
			       "bthcrd", 0);
		if (sc->sc_dying)
			error = EIO;
		if (error) {
			splx(s);
			DPRINTFN(0, ("%s: tsleep() = %d\n",
				     __func__, error));
			goto ret;
		}
	}
	splx(s);

	if (uio->uio_resid < sc->sc_rd_len) {
#ifdef DIAGNOSTIC
		printf("bthciread: short read %ld < %ld\n",
		    (long)uio->uio_resid, (long)sc->sc_rd_len);
#endif
		error = EINVAL;
		goto ret;
	}

	error = uiomove(sc->sc_rd_buf, sc->sc_rd_len, uio);

	if (!error) {
		sc->sc_outputready = 0;
		sc->sc_rd_len = 0;

		/* Unblock events (in case they were blocked) */
		sc->sc_methods->bt_unblockcb(sc->sc_handle,
					     BT_CBBLOCK_ACL_DATA |
					     BT_CBBLOCK_EVENT);
	}

 ret:
	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);

	return error;
}

static int
bthcichanwrite(struct bthci_softc *sc,
	       struct btframe_channel const *btchannel, struct uio *uio)
{
	struct btframe_buffer *btframebuf;
	u_int8_t *btbuf;
	int error;
	size_t uiolen;

	sc->sc_refcnt++;

	uiolen = uio->uio_resid;

	btbuf = btchannel->bt_alloc(sc->sc_handle, uiolen, &btframebuf);
	if (btbuf == NULL) {
		error = ENOMEM;
		goto ret;
	}

	error = uiomove(btbuf, uiolen, uio);
	if (error)
		goto ret;

	error = btchannel->bt_send(sc->sc_handle, btframebuf, uiolen);

 ret:
	if (--sc->sc_refcnt < 0)
		wakeup(&sc->sc_refcnt);

	return error;
}

static int
bthciwrite(dev_t dev, struct uio *uio, int flag)
{
	struct bthci_softc *sc;
	struct btframe_channel const *btchannel;
	int error;
	size_t uiolen;
	u_int8_t bthcitype;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);

	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);

	uiolen = uio->uio_resid;

	DPRINTFN(1, ("%s: length=%lu\n", __func__, (unsigned long)uiolen));

	if (uiolen > BTHCI_ACL_DATA_MAX_LEN + 1) {
#ifdef DIAGNOSTIC
		printf("bthciread: long write %ld\n", (long)uio->uio_resid);
#endif
		return (EINVAL);
	}

	if (uiolen <= 1) {
#ifdef DIAGNOSTIC
		printf("bthciread: short write %ld\n", (long)uio->uio_resid);
#endif
		return (EINVAL);
	}

	error = uiomove(&bthcitype, 1, uio);
	if (error)
		return (error);

	if (bthcitype == BTHCI_PKTID_COMMAND) {
		/* Command */
		btchannel = &sc->sc_methods->bt_control;
	} else if (bthcitype == BTHCI_PKTID_ACL_DATA) {
		/* ACL data */
		btchannel = &sc->sc_methods->bt_acldata;
	} else if (bthcitype == BTHCI_PKTID_SCO_DATA) {
		/* SCO data */
		btchannel = &sc->sc_methods->bt_scodata;
	} else {
		/* Bad packet type */
		return (EINVAL);
	}

	return (bthcichanwrite(sc, btchannel, uio));
}

#if 0
int
bthciioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	struct bthci_softc *sc;
	void *vaddr = addr;
	int error;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);

	switch (cmd) {
	case FIONBIO:
		/* All handled in the upper FS layer. */
		error = 0;
		break;

	default:
		error = EINVAL;
		break;
	}
	return (error);
}
#endif

int
bthcipoll(dev_t dev, int events, struct proc *p)
{
	struct bthci_softc *sc;
	int revents;
	int s;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);

	revents = events & (POLLOUT | POLLWRNORM);

	if (events & (POLLIN | POLLRDNORM)) {
		s = sc->sc_methods->bt_splraise();
		if (sc->sc_outputready) {
			DPRINTFN(2,("%s: have data\n", __func__));
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			DPRINTFN(2,("%s: recording read select\n",
				    __func__));
			selrecord(p, &sc->sc_rd_sel);
		}
		splx(s);
	}

	return revents;
}

static void
filt_bthcirdetach(struct knote *kn)
{
	struct bthci_softc *sc = kn->kn_hook;
	int s;

	s = sc->sc_methods->bt_splraise();
	SLIST_REMOVE(&sc->sc_rd_sel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

/* ARGSUSED */
static int
filt_bthciread(struct knote *kn, long hint)
{
	struct bthci_softc *sc = kn->kn_hook;

	kn->kn_data = sc->sc_rd_len;
	return (kn->kn_data > 0);
}

static const struct filterops bthciread_filtops =
	{ 1, NULL, filt_bthcirdetach, filt_bthciread };

int
bthcikqfilter(dev_t dev, struct knote *kn)
{
	struct bthci_softc *sc;
	struct klist *klist;
	int s;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rd_sel.sel_klist;
		kn->kn_fop = &bthciread_filtops;
		break;
	default:
		return (1);
	}

	kn->kn_hook = sc;

	s = sc->sc_methods->bt_splraise();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

static void
bthci_recveventdata(void *h, u_int8_t *data, size_t len)
{
	struct bthci_softc *sc = h;
	size_t pktlength;
	unsigned int ecode;

	if (len < BTHCI_EVENT_MIN_LEN || len > BTHCI_EVENT_MAX_LEN) {
		DPRINTFN(1,("%s: invalid sized event, size=%u\n",
			    __func__, (unsigned int)len));
		return;
	}

	ecode = data[0];
	DPRINTFN(1,("%s: received event: %02x (%s)\n", __func__,
		    ecode, bthci_eventstr(ecode)));

	pktlength = data[BTHCI_EVENT_LEN_OFFT] + BTHCI_EVENT_LEN_OFFT +
		BTHCI_EVENT_LEN_LENGTH;
	if (pktlength != len) {
		DPRINTFN(1,("%s: event length didn't match, "
			    "pktlen=%u size=%u\n",
			    __func__, (unsigned int)pktlength,
			    (unsigned int)len));
		return;
	}

	if (!sc->sc_open)
		return;

	if (sc->sc_rd_len != 0) {
		DPRINTFN(1,("%s: dropping an event, size=%u\n",
			    __func__, (unsigned int)len));
		return;
	}

	sc->sc_rd_buf[0] = BTHCI_PKTID_EVENT;
	memcpy(&sc->sc_rd_buf[1], data, len);
	sc->sc_rd_len = len + 1;
	sc->sc_outputready = 1;

	sc->sc_methods->bt_blockcb(sc->sc_handle,
				   BT_CBBLOCK_ACL_DATA | BT_CBBLOCK_EVENT);

	wakeup(&sc->sc_outputready);
	selnotify(&sc->sc_rd_sel, 0);
}

static void
bthci_recvacldata(void *h, u_int8_t *data, size_t len)
{
	struct bthci_softc *sc = h;
	size_t pktlength;

	if (!sc->sc_open)
		return;

	if (len < BTHCI_ACL_DATA_MIN_LEN || len > BTHCI_ACL_DATA_MAX_LEN) {
		DPRINTFN(1,("%s: invalid acl packet, size=%u\n",
			    __func__, (unsigned int)len));
		return;
	}

	pktlength = BTGETW(&data[BTHCI_ACL_DATA_LEN_OFFT]) +
		BTHCI_ACL_DATA_LEN_OFFT +
		BTHCI_ACL_DATA_LEN_LENGTH;

	if (pktlength != len) {
		DPRINTFN(1,("%s: acl packet length didn't match, "
			    "pktlen=%u size=%u\n",
			    __func__, (unsigned int)pktlength,
			    (unsigned int)len));
		return;
	}

	if (sc->sc_rd_len != 0) {
		DPRINTFN(1,("%s: dropping an acl packet, size=%u\n",
			    __func__, (unsigned int)len));
		return;
	}

	sc->sc_rd_buf[0] = BTHCI_PKTID_ACL_DATA;
	memcpy(&sc->sc_rd_buf[1], data, len);
	sc->sc_rd_len = len + 1;
	sc->sc_outputready = 1;

	sc->sc_methods->bt_blockcb(sc->sc_handle,
				   BT_CBBLOCK_ACL_DATA | BT_CBBLOCK_EVENT);

	wakeup(&sc->sc_outputready);
	selnotify(&sc->sc_rd_sel, 0);
}

static void
bthci_recvscodata(void *h, u_int8_t *data, size_t len)
{
	struct bthci_softc *sc = h;

	if (!sc->sc_open)
		return;
}
