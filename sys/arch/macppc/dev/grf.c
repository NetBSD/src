/*	$NetBSD: grf.c,v 1.1 1998/05/15 10:15:48 tsubai Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: grf.c 1.31 91/01/21$
 *
 *	@(#)grf.c	7.8 (Berkeley) 5/7/91
 */

/*
 * Graphics display driver for the PowerMacintosh.
 * This is the hardware-independent portion of the driver.
 * Hardware access is through the grfdev routines below.
 */

#include <sys/param.h>

#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/systm.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/grfioctl.h>

#include <macppc/dev/itevar.h>
#include <macppc/dev/grfvar.h>

#include "grf.h"
#include "ite.h"

#if NITE == 0
#define	iteon(u,f)
#define	iteoff(u,f)
#endif

int	grfmatch __P((struct device *, struct cfdata *, void *));
void	grfattach __P((struct device *, struct device *, void *));
int grfbusprint();

struct cfattach grf_ca = {
	sizeof(struct grf_softc), grfmatch, grfattach
};

extern struct cfdriver grf_cd;

#ifdef DEBUG
#define	GRF_DEBUG
#endif

#ifdef GRF_DEBUG
#define GDB_DEVNO	0x01
#define GDB_MMAP	0x02
#define GDB_IOMAP	0x04
#define GDB_LOCK	0x08
int grfdebug = 0;
#endif

int
grfmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct grf_attach_args *ga = aux;

	if (strcmp(ga->ga_name, "grf") != 0)
		return 0;
	return 1;
}

void
grfattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct grf_softc *sc = (struct grf_softc *)self;
	struct grf_attach_args *ga = aux;

	printf("\n");

	/* Load forwarded pointers. */
	sc->sc_display = ga->ga_display;
	sc->sc_mode = ga->ga_mode;

	sc->sc_flags = GF_ALIVE;	/* XXX bogus */

	/*
	 * Attach ite semantics to the grf.  Change the name, forward
	 * everything else.
	 */
	ga->ga_name = "ite";
	config_found(self, ga, grfbusprint);
}

/*ARGSUSED*/
int
grfopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	register struct grf_softc *gp;
	int unit;
	int error = 0;

	unit = GRFUNIT(dev);
	gp = grf_cd.cd_devs[unit];

	if (unit >= grf_cd.cd_ndevs || (gp->sc_flags & GF_ALIVE) == 0)
		return (ENXIO);

	if ((gp->sc_flags & (GF_OPEN | GF_EXCLUDE)) == (GF_OPEN | GF_EXCLUDE))
		return (EBUSY);

	/*
	 * First open.
	 * XXX: always put in graphics mode.
	 */
	if ((gp->sc_flags & GF_OPEN) == 0) {
		gp->sc_flags |= GF_OPEN;
		/*error = grfon(dev);*/
	}
	return (error);
}

/*ARGSUSED*/
int
grfclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	register struct grf_softc *gp;

	gp = grf_cd.cd_devs[GRFUNIT(dev)];

	/*(void)grfoff(dev);*/
	gp->sc_flags &= GF_ALIVE;

	return (0);
}

/*ARGSUSED*/
int
grfioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct grf_softc *gp;
	struct grfinfo *gm;
	int     error;
	int	unit = GRFUNIT(dev);

	gp = grf_cd.cd_devs[unit];
	gm = gp->sc_display;
	error = 0;

	switch (cmd) {
	case GRFIOCGINFO:
		bcopy(gm, data, sizeof(struct grfinfo));
		break;
	case GRFIOCON:
		error = grfon(dev);
		break;
	case GRFIOCOFF:
		error = grfoff(dev);
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

/*ARGSUSED*/
int
grfpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	return (events & (POLLOUT | POLLWRNORM));
}

/*ARGSUSED*/
int
grfmmap(dev, off, prot)
	dev_t dev;
	int off;
	int prot;
{
	struct grf_softc *gp = grf_cd.cd_devs[GRFUNIT(dev)];
	struct grfinfo *gi = gp->sc_display;

	if (off >= 0 && off < gi->gd_devsize)
		return (int)gi->gd_devaddr + off;

	return -1;
}

int
grfon(dev)
	dev_t   dev;
{
	int     unit = GRFUNIT(dev);
	struct grf_softc *gp;

	gp = grf_cd.cd_devs[unit];

	/*
	 * XXX: iteoff call relies on devices being in same order
	 * as ITEs and the fact that iteoff only uses the minor part
	 * of the dev arg.
	 */
	iteoff(unit, 3);

	return (*gp->sc_mode)(gp, GM_GRFON, NULL);
}

int
grfoff(dev)
	dev_t   dev;
{
	int     unit = GRFUNIT(dev);
	struct grf_softc *gp;
	int     error;

	gp = grf_cd.cd_devs[unit];

	error = (*gp->sc_mode)(gp, GM_GRFOFF, NULL);

	/* XXX: see comment for iteoff above */
	iteon(unit, 2);

	return (error);
}
