/*	$NetBSD: irframe.c,v 1.15.4.3 2002/02/11 20:09:51 jdolecek Exp $	*/

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

#include "irframe.h"

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

#include <dev/ir/ir.h>
#include <dev/ir/irdaio.h>
#include <dev/ir/irframevar.h>

#ifdef IRFRAME_DEBUG
#define DPRINTF(x)	if (irframedebug) printf x
#define Static
int irframedebug = 0;
#else
#define DPRINTF(x)
#define Static static
#endif

cdev_decl(irframe);

int irframe_match(struct device *parent, struct cfdata *match, void *aux);
void irframe_attach(struct device *parent, struct device *self, void *aux);
int irframe_activate(struct device *self, enum devact act);
int irframe_detach(struct device *self, int flags);

Static int irf_set_params(struct irframe_softc *sc, struct irda_params *p);
Static int irf_reset_params(struct irframe_softc *sc);

#if NIRFRAME == 0
/* In case we just have tty attachment. */
struct cfdriver irframe_cd = {
	NULL, "irframe", DV_DULL
};
#endif

struct cfattach irframe_ca = {
	sizeof(struct irframe_softc), irframe_match, irframe_attach,
	irframe_detach, irframe_activate
};

extern struct cfattach irframe_ca;
extern struct cfdriver irframe_cd;

#define IRFRAMEUNIT(dev) (minor(dev))

int
irframe_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct ir_attach_args *ia = aux;

	return (ia->ia_type == IR_TYPE_IRFRAME);
}

void
irframe_attach(struct device *parent, struct device *self, void *aux)
{
	struct irframe_softc *sc = (struct irframe_softc *)self;
	struct ir_attach_args *ia = aux;
	const char *delim;
	int speeds = 0;

	sc->sc_methods = ia->ia_methods;
	sc->sc_handle = ia->ia_handle;

#ifdef DIAGNOSTIC
	if (sc->sc_methods->im_read == NULL ||
	    sc->sc_methods->im_write == NULL ||
	    sc->sc_methods->im_poll == NULL ||
	    sc->sc_methods->im_kqfilter == NULL ||
	    sc->sc_methods->im_set_params == NULL ||
	    sc->sc_methods->im_get_speeds == NULL ||
	    sc->sc_methods->im_get_turnarounds == NULL)
		panic("%s: missing methods\n", sc->sc_dev.dv_xname);
#endif

	(void)sc->sc_methods->im_get_speeds(sc->sc_handle, &speeds);
	sc->sc_speedmask = speeds;
	delim = ":";
	if (speeds & IRDA_SPEEDS_SIR) {
		printf("%s SIR", delim);
		delim = ",";
	}
	if (speeds & IRDA_SPEEDS_MIR) {
		printf("%s MIR", delim);
		delim = ",";
	}
	if (speeds & IRDA_SPEEDS_FIR) {
		printf("%s FIR", delim);
		delim = ",";
	}
	if (speeds & IRDA_SPEEDS_VFIR) {
		printf("%s VFIR", delim);
		delim = ",";
	}
	printf("\n");
}

int
irframe_activate(struct device *self, enum devact act)
{
	/*struct irframe_softc *sc = (struct irframe_softc *)self;*/

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
irframe_detach(struct device *self, int flags)
{
	/*struct irframe_softc *sc = (struct irframe_softc *)self;*/
	int maj, mn;

	/* XXX needs reference count */

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == irframeopen)
			break;

	/* Nuke the vnodes for any open instances (calls close). */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);

	return (0);
}

int
irframeopen(dev_t dev, int flag, int mode, struct proc *p)
{
	struct irframe_softc *sc;
	int error;

	sc = device_lookup(&irframe_cd, IRFRAMEUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (EIO);
	if (sc->sc_open)
		return (EBUSY);
	if (sc->sc_methods->im_open != NULL) {
		error = sc->sc_methods->im_open(sc->sc_handle, flag, mode, p);
		if (error)
			return (error);
	}
	sc->sc_open = 1;
#ifdef DIAGNOSTIC
	sc->sc_speed = IRDA_DEFAULT_SPEED;
#endif
	(void)irf_reset_params(sc);
	return (0);
}

int
irframeclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct irframe_softc *sc;
	int error;

	sc = device_lookup(&irframe_cd, IRFRAMEUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	sc->sc_open = 0;
	if (sc->sc_methods->im_close != NULL)
		error = sc->sc_methods->im_close(sc->sc_handle, flag, mode, p);
	else
		error = 0;
	return (error);
}

int
irframeread(dev_t dev, struct uio *uio, int flag)
{
	struct irframe_softc *sc;

	sc = device_lookup(&irframe_cd, IRFRAMEUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);
	if (uio->uio_resid < sc->sc_params.maxsize) {
#ifdef DIAGNOSTIC
		printf("irframeread: short read %d < %d\n", uio->uio_resid,
		       sc->sc_params.maxsize);
#endif
		return (EINVAL);
	}
	return (sc->sc_methods->im_read(sc->sc_handle, uio, flag));
}

int
irframewrite(dev_t dev, struct uio *uio, int flag)
{
	struct irframe_softc *sc;

	sc = device_lookup(&irframe_cd, IRFRAMEUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);
	if (uio->uio_resid > sc->sc_params.maxsize) {
#ifdef DIAGNOSTIC
		printf("irframeread: long write %d > %d\n", uio->uio_resid,
		       sc->sc_params.maxsize);
#endif
		return (EINVAL);
	}
	return (sc->sc_methods->im_write(sc->sc_handle, uio, flag));
}

int
irf_set_params(struct irframe_softc *sc, struct irda_params *p)
{
	int error;

	DPRINTF(("irf_set_params: set params speed=%u ebofs=%u maxsize=%u "
		 "speedmask=0x%x\n", p->speed, p->ebofs, p->maxsize,
		 sc->sc_speedmask));

	if (p->maxsize > IRDA_MAX_FRAME_SIZE) {
#ifdef IRFRAME_DEBUG
		printf("irf_set_params: bad maxsize=%u\n", p->maxsize);
#endif
		return (EINVAL);
	}

	if (p->ebofs > IRDA_MAX_EBOFS) {
#ifdef IRFRAME_DEBUG
		printf("irf_set_params: bad maxsize=%u\n", p->maxsize);
#endif
		return (EINVAL);
	}

#define CONC(x,y) x##y
#define CASE(s) case s: if (!(sc->sc_speedmask & CONC(IRDA_SPEED_,s))) return (EINVAL); break
	switch (p->speed) {
	CASE(2400);
	CASE(9600);
	CASE(19200);
	CASE(38400);
	CASE(57600);
	CASE(115200);
	CASE(576000);
	CASE(1152000);
	CASE(4000000);
	CASE(16000000);
	default: return (EINVAL);
	}
#undef CONC
#undef CASE

	error = sc->sc_methods->im_set_params(sc->sc_handle, p);
	if (!error) {
		sc->sc_params = *p;
		DPRINTF(("irf_set_params: ok\n"));
#ifdef DIAGNOSTIC
		if (p->speed != sc->sc_speed) {
			sc->sc_speed = p->speed;
			printf("%s: set speed %u\n", sc->sc_dev.dv_xname,
			       sc->sc_speed);
		}
#endif
	} else {
#ifdef IRFRAME_DEBUG
		printf("irf_set_params: error=%d\n", error);
#endif
	}
	return (error);
}

int
irf_reset_params(struct irframe_softc *sc)
{
	struct irda_params params;

	params.speed = IRDA_DEFAULT_SPEED;
	params.ebofs = IRDA_DEFAULT_EBOFS;
	params.maxsize = IRDA_DEFAULT_SIZE;
	return (irf_set_params(sc, &params));
}

int
irframeioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	struct irframe_softc *sc;
	void *vaddr = addr;
	int error;

	sc = device_lookup(&irframe_cd, IRFRAMEUNIT(dev));
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

int
irframepoll(dev_t dev, int events, struct proc *p)
{
	struct irframe_softc *sc;

	sc = device_lookup(&irframe_cd, IRFRAMEUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (EIO);

	return (sc->sc_methods->im_poll(sc->sc_handle, events, p));
}

int
irframekqfilter(dev_t dev, struct knote *kn)
{
	struct irframe_softc *sc;

	sc = device_lookup(&irframe_cd, IRFRAMEUNIT(dev));
	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 || !sc->sc_open)
		return (1);

	return (sc->sc_methods->im_kqfilter(sc->sc_handle, kn));
}


/*********/


struct device *
irframe_alloc(size_t size, const struct irframe_methods *m, void *h)
{
	struct cfdriver *cd = &irframe_cd;
	struct device *dev;
	struct ir_attach_args ia;
	int unit;

	for (unit = 0; unit < cd->cd_ndevs; unit++)
		if (cd->cd_devs[unit] == NULL)
			break;
	dev = malloc(size, M_DEVBUF, M_WAITOK|M_ZERO);
	snprintf(dev->dv_xname, sizeof dev->dv_xname, "irframe%d", unit);
	dev->dv_unit = unit;
	dev->dv_flags = DVF_ACTIVE;	/* always initially active */

	config_makeroom(unit, cd);
	cd->cd_devs[unit] = dev;

	ia.ia_methods = m;
	ia.ia_handle = h;
	printf("%s", dev->dv_xname);
	irframe_attach(NULL, dev, &ia);

	return (dev);
}

void
irframe_dealloc(struct device *dev)
{
	struct cfdriver *cd = &irframe_cd;
	int unit;

	for (unit = 0; unit < cd->cd_ndevs; unit++) {
		if (cd->cd_devs[unit] == dev) {
			cd->cd_devs[unit] = NULL;
			free(dev, M_DEVBUF);
			return;
		}
	}
	panic("irframe_dealloc: device not found\n");
}
