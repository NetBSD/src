/*	$NetBSD: cir.c,v 1.24 2009/05/12 12:18:09 cegger Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cir.c,v 1.24 2009/05/12 12:18:09 cegger Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/vnode.h>

#include <dev/ir/ir.h>
#include <dev/ir/cirio.h>
#include <dev/ir/cirvar.h>

dev_type_open(ciropen);
dev_type_close(circlose);
dev_type_read(cirread);
dev_type_write(cirwrite);
dev_type_ioctl(cirioctl);
dev_type_poll(cirpoll);

const struct cdevsw cir_cdevsw = {
	ciropen, circlose, cirread, cirwrite, cirioctl,
	nostop, notty, cirpoll, nommap, nokqfilter,
	D_OTHER
};

int cir_match(struct device *parent, cfdata_t match, void *aux);
void cir_attach(struct device *parent, struct device *self, void *aux);
int cir_activate(struct device *self, enum devact act);
int cir_detach(struct device *self, int flags);

CFATTACH_DECL(cir, sizeof(struct cir_softc),
    cir_match, cir_attach, cir_detach, cir_activate);

extern struct cfdriver cir_cd;

#define CIRUNIT(dev) (minor(dev))

int
cir_match(struct device *parent, cfdata_t match, void *aux)
{
	struct ir_attach_args *ia = aux;

	return (ia->ia_type == IR_TYPE_CIR);
}

void
cir_attach(struct device *parent, struct device *self, void *aux)
{
	struct cir_softc *sc = device_private(self);
	struct ir_attach_args *ia = aux;

	selinit(&sc->sc_rdsel);
	sc->sc_methods = ia->ia_methods;
	sc->sc_handle = ia->ia_handle;

#ifdef DIAGNOSTIC
	if (sc->sc_methods->im_read == NULL ||
	    sc->sc_methods->im_write == NULL ||
	    sc->sc_methods->im_setparams == NULL)
		panic("%s: missing methods", device_xname(&sc->sc_dev));
#endif
	printf("\n");
}

int
cir_activate(struct device *self, enum devact act)
{
	/*struct cir_softc *sc = device_private(self);*/

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
cir_detach(struct device *self, int flags)
{
	struct cir_softc *sc = device_private(self);
	int maj, mn;

	/* locate the major number */
	maj = cdevsw_lookup_major(&cir_cdevsw);

	/* Nuke the vnodes for any open instances (calls close). */
	mn = device_unit(self);
	vdevgone(maj, mn, mn, VCHR);

	seldestroy(&sc->sc_rdsel);

	return (0);
}

int
ciropen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct cir_softc *sc;
	int error;

	sc = device_lookup_private(&cir_cd, CIRUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if (!device_is_active(&sc->sc_dev))
		return (EIO);
	if (sc->sc_open)
		return (EBUSY);

	sc->sc_rdframes = 0;
	if (sc->sc_methods->im_open != NULL) {
		error = sc->sc_methods->im_open(sc->sc_handle, flag, mode,
		    l->l_proc);
		if (error)
			return (error);
	}
	sc->sc_open = 1;
	return (0);
}

int
circlose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct cir_softc *sc;
	int error;

	sc = device_lookup_private(&cir_cd, CIRUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if (sc->sc_methods->im_close != NULL)
		error = sc->sc_methods->im_close(sc->sc_handle, flag, mode,
		    l->l_proc);
	else
		error = 0;
	sc->sc_open = 0;
	return (error);
}

int
cirread(dev_t dev, struct uio *uio, int flag)
{
	struct cir_softc *sc;

	sc = device_lookup_private(&cir_cd, CIRUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if (!device_is_active(&sc->sc_dev))
		return (EIO);
	return (sc->sc_methods->im_read(sc->sc_handle, uio, flag));
}

int
cirwrite(dev_t dev, struct uio *uio, int flag)
{
	struct cir_softc *sc;

	sc = device_lookup_private(&cir_cd, CIRUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if (!device_is_active(&sc->sc_dev))
		return (EIO);
	return (sc->sc_methods->im_write(sc->sc_handle, uio, flag));
}

int
cirioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct cir_softc *sc;
	int error;

	sc = device_lookup_private(&cir_cd, CIRUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if (!device_is_active(&sc->sc_dev))
		return (EIO);

	switch (cmd) {
	case FIONBIO:
		/* All handled in the upper FS layer. */
		error = 0;
		break;
	case CIR_GET_PARAMS:
		*(struct cir_params *)addr = sc->sc_params;
		error = 0;
		break;
	case CIR_SET_PARAMS:
		error = sc->sc_methods->im_setparams(sc->sc_handle,
			    (struct cir_params *)addr);
		if (!error)
			sc->sc_params = *(struct cir_params *)addr;
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

int
cirpoll(dev_t dev, int events, struct lwp *l)
{
	struct cir_softc *sc;
	int revents;
	int s;

	sc = device_lookup_private(&cir_cd, CIRUNIT(dev));
	if (sc == NULL)
		return (POLLERR);
	if (!device_is_active(&sc->sc_dev))
		return (POLLERR);

	revents = 0;
	s = splir();
	if (events & (POLLIN | POLLRDNORM))
		if (sc->sc_rdframes > 0)
			revents |= events & (POLLIN | POLLRDNORM);

#if 0
	/* How about write? */
	if (events & (POLLOUT | POLLWRNORM))
		if (/* ??? */)
			revents |= events & (POLLOUT | POLLWRNORM);
#endif

	if (revents == 0) {
		if (events & (POLLIN | POLLRDNORM))
			selrecord(l, &sc->sc_rdsel);

#if 0
		if (events & (POLLOUT | POLLWRNORM))
			selrecord(p, &sc->sc_wrsel);
#endif
	}

	splx(s);
	return (revents);
}
