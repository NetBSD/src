/*	$NetBSD: bthci.c,v 1.3 2002/09/12 06:42:54 augustss Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
#include <dev/bluetooth/bthcivar.h>

#ifdef BTHCI_DEBUG
#define DPRINTF(x)	if (bthcidebug) printf x
#define Static
int bthcidebug = 0;
#else
#define DPRINTF(x)
#define Static static
#endif

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

struct cfattach bthci_ca = {
	sizeof(struct bthci_softc), bthci_match, bthci_attach,
	bthci_detach, bthci_activate
};

extern struct cfattach bthci_ca;
extern struct cfdriver bthci_cd;

dev_type_open(bthciopen);
dev_type_close(bthciclose);
dev_type_poll(bthcipoll);

const struct cdevsw bthci_cdevsw = {
	bthciopen, bthciclose, noread, nowrite, noioctl,
	nostop, notty, bthcipoll, nommap,
};

#define BTHCIUNIT(dev) (minor(dev))

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

#ifdef DIAGNOSTIC_XXX
	if (sc->sc_methods->bt_read == NULL ||
	    sc->sc_methods->bt_write == NULL ||
	    sc->sc_methods->bt_poll == NULL)
		panic("%s: missing methods\n", sc->sc_dev.dv_xname);
#endif

	printf("driver not implemented");
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

int
bthci_detach(struct device *self, int flags)
{
	/*struct bthci_softc *sc = (struct bthci_softc *)self;*/
	int maj, mn;

	/* XXX needs reference count */

	/* locate the major number */
	maj = cdevsw_lookup_major(&bthci_cdevsw);

	/* Nuke the vnodes for any open instances (calls close). */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);

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
	if (sc->sc_methods->bt_open != NULL) {
		error = sc->sc_methods->bt_open(sc->sc_handle, flag, mode, p);
		if (error)
			return (error);
	}
	sc->sc_open = 1;
	return (0);
}

int
bthciclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct bthci_softc *sc;
	int error;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	sc->sc_open = 0;
	if (sc->sc_methods->bt_close != NULL)
		error = sc->sc_methods->bt_close(sc->sc_handle, flag, mode, p);
	else
		error = 0;
	return (error);
}

#if 0
int
bthciread(dev_t dev, struct uio *uio, int flag)
{
	struct bthci_softc *sc;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);
	if (uio->uio_resid < sc->sc_params.maxsize) {
#ifdef DIAGNOSTIC
		printf("bthciread: short read %d < %d\n", uio->uio_resid,
		       sc->sc_params.maxsize);
#endif
		return (EINVAL);
	}
	return (sc->sc_methods->im_read(sc->sc_handle, uio, flag));
}

int
bthciwrite(dev_t dev, struct uio *uio, int flag)
{
	struct bthci_softc *sc;

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);
	if (uio->uio_resid > sc->sc_params.maxsize) {
#ifdef DIAGNOSTIC
		printf("bthciread: long write %d > %d\n", uio->uio_resid,
		       sc->sc_params.maxsize);
#endif
		return (EINVAL);
	}
	return (sc->sc_methods->im_write(sc->sc_handle, uio, flag));
}
#endif

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

	case IRDA_SET_PARAMS:
		error = irf_set_params(sc, vaddr);
		break;

	case IRDA_RESET_PARAMS:
		error = irf_reset_params(sc);
		break;

	case IRDA_GET_SPEEDMASK:
		error = sc->sc_methods->im_get_speeds(sc->sc_handle, vaddr);
		break;

	case IRDA_GET_TURNAROUNDMASK:
		error = sc->sc_methods->im_get_turnarounds(sc->sc_handle,vaddr);
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

	sc = device_lookup(&bthci_cd, BTHCIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);

	return (sc->sc_methods->bt_poll(sc->sc_handle, events, p));
}

