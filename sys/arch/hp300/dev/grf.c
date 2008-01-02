/*	$NetBSD: grf.c,v 1.61.32.1 2008/01/02 21:47:46 bouyer Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: grf.c 1.36 93/08/13$
 *
 *	@(#)grf.c	8.4 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 * from: Utah $Hdr: grf.c 1.36 93/08/13$
 *
 *	@(#)grf.c	8.4 (Berkeley) 1/12/94
 */

/*
 * Graphics display driver for HP 300/400/700/800 machines.
 * This is the hardware-independent portion of the driver.
 * Hardware access is through the machine dependent grf switch routines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf.c,v 1.61.32.1 2008/01/02 21:47:46 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/resourcevar.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <hp300/dev/intioreg.h>
#include <hp300/dev/grfioctl.h>
#include <hp300/dev/grfvar.h>
#include <hp300/dev/grfreg.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>

#include <miscfs/specfs/specdev.h>

#include "ite.h"
#if NITE > 0
#include <hp300/dev/itevar.h>
#else
#define	iteon(u,f)		0	/* noramlly returns int */
#define	iteoff(u,f)
#endif /* NITE > 0 */

#include "ioconf.h"

static int	grfmatch(struct device *, struct cfdata *, void *);
static void	grfattach(struct device *, struct device *, void *);

CFATTACH_DECL(grf, sizeof(struct grf_softc),
    grfmatch, grfattach, NULL, NULL);

static dev_type_open(grfopen);
static dev_type_close(grfclose);
static dev_type_ioctl(grfioctl);
static dev_type_mmap(grfmmap);

const struct cdevsw grf_cdevsw = {
	grfopen, grfclose, nullread, nullwrite, grfioctl,
	nostop, notty, nopoll, grfmmap, nokqfilter,
};

static int	grfprint(void *, const char *);

/*
 * Frambuffer state information, statically allocated for benefit
 * of the console.
 */
struct	grf_data grf_cn;

#ifdef DEBUG
int grfdebug = 0;
#define GDB_DEVNO	0x01
#define GDB_MMAP	0x02
#define GDB_IOMAP	0x04
#define GDB_LOCK	0x08
#endif

static int
grfmatch(struct device *parent, struct cfdata *match, void *aux)
{

	return 1;
}

static void
grfattach(struct device *parent, struct device *self, void *aux)
{
	struct grf_softc *sc = (struct grf_softc *)self;
	struct grfdev_attach_args *ga = aux;

	printf("\n");

	sc->sc_data = ga->ga_data;
	sc->sc_scode = ga->ga_scode;	/* XXX */

	/* Attach an ITE. */
	(void)config_found(self, aux, grfprint);
}

static int
grfprint(void *aux, const char *pnp)
{

	/* Only ITEs can attach to GRFs, easy... */
	if (pnp)
		aprint_normal("ite at %s", pnp);

	return UNCONF;
}

/*ARGSUSED*/
static int
grfopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = GRFUNIT(dev);
	struct grf_softc *sc;
	struct grf_data *gp;
	int error = 0;

	if (unit >= grf_cd.cd_ndevs ||
	    (sc = grf_cd.cd_devs[unit]) == NULL)
		return ENXIO;

	gp = sc->sc_data;

	if ((gp->g_flags & GF_ALIVE) == 0)
		return ENXIO;

	if ((gp->g_flags & (GF_OPEN|GF_EXCLUDE)) == (GF_OPEN|GF_EXCLUDE))
		return EBUSY;
	/*
	 * First open.
	 * XXX: always put in graphics mode.
	 */
	error = 0;
	if ((gp->g_flags & GF_OPEN) == 0) {
		gp->g_flags |= GF_OPEN;
		error = grfon(dev);
	}
	return error;
}

/*ARGSUSED*/
static int
grfclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = GRFUNIT(dev);
	struct grf_softc *sc;
	struct grf_data *gp;

	sc = grf_cd.cd_devs[unit];

	gp = sc->sc_data;

	if ((gp->g_flags & GF_ALIVE) == 0)
		return ENXIO;

	(void) grfoff(dev);
	gp->g_flags &= GF_ALIVE;
	return 0;
}

/*ARGSUSED*/
static int
grfioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct grf_softc *sc;
	struct grf_data *gp;
	int error, unit = GRFUNIT(dev);

	sc = grf_cd.cd_devs[unit];

	gp = sc->sc_data;

	if ((gp->g_flags & GF_ALIVE) == 0)
		return ENXIO;

	error = 0;
	switch (cmd) {

	case GRFIOCGINFO:
		memcpy(data, (void *)&gp->g_display, sizeof(struct grfinfo));
		break;

	case GRFIOCON:
		error = grfon(dev);
		break;

	case GRFIOCOFF:
		error = grfoff(dev);
		break;

	case GRFIOCMAP:
		error = grfmap(dev, (void **)data, l->l_proc);
		break;

	case GRFIOCUNMAP:
		error = grfunmap(dev, *(void **)data, l->l_proc);
		break;

	default:
		error = EINVAL;
		break;

	}
	return error;
}

/*ARGSUSED*/
static paddr_t
grfmmap(dev_t dev, off_t off, int prot)
{
	struct grf_softc *sc = grf_cd.cd_devs[GRFUNIT(dev)];

	return grfaddr(sc, off);
}

int
grfon(dev_t dev/*XXX*/)
{
	int unit = GRFUNIT(dev);
	struct grf_softc *sc;
	struct grf_data *gp;

	sc = grf_cd.cd_devs[unit];
	gp = sc->sc_data;

	/*
	 * XXX: iteoff call relies on devices being in same order
	 * as ITEs and the fact that iteoff only uses the minor part
	 * of the dev arg.
	 */
	iteoff(sc->sc_ite->sc_data, 3);
	return (*gp->g_sw->gd_mode)(gp,
	    (dev & GRFOVDEV) ? GM_GRFOVON : GM_GRFON, (void *)0);
}

int
grfoff(dev_t dev/*XXX*/)
{
	int unit = GRFUNIT(dev);
	struct grf_softc *sc;
	struct grf_data *gp;
	int error;

	sc = grf_cd.cd_devs[unit];
	gp = sc->sc_data;

	(void) grfunmap(dev, (void *)0, curproc);
	error = (*gp->g_sw->gd_mode)(gp,
				     (dev&GRFOVDEV) ? GM_GRFOVOFF : GM_GRFOFF,
				     (void *)0);
	/* XXX: see comment for iteoff above */
	(void) iteon(sc->sc_ite->sc_data, 2);
	return error;
}

paddr_t
grfaddr(struct grf_softc *sc, off_t off)
{
	struct grf_data *gp= sc->sc_data;
	struct grfinfo *gi = &gp->g_display;

	/* control registers */
	if (off >= 0 && off < gi->gd_regsize)
		return ((paddr_t)gi->gd_regaddr + off) >> PGSHIFT;

	/* frame buffer */
	if (off >= gi->gd_regsize && off < gi->gd_regsize+gi->gd_fbsize) {
		off -= gi->gd_regsize;
		return ((paddr_t)gi->gd_fbaddr + off) >> PGSHIFT;
	}
	/* bogus */
	return -1;
}

int
grfmap(dev_t dev, void **addrp, struct proc *p)
{
	struct grf_softc *sc = grf_cd.cd_devs[GRFUNIT(dev)];
	struct grf_data *gp = sc->sc_data;
	int len, error;
	struct vnode vn;
	struct specinfo si;
	int flags;

#ifdef DEBUG
	if (grfdebug & GDB_MMAP)
		printf("grfmap(%d): addr %p\n", p->p_pid, *addrp);
#endif
	len = gp->g_display.gd_regsize + gp->g_display.gd_fbsize;
	flags = MAP_SHARED;
	if (*addrp)
		flags |= MAP_FIXED;
	else
		*addrp = (void *)
		    VM_DEFAULT_ADDRESS(p->p_vmspace->vm_daddr, len);
	vn.v_type = VCHR;			/* XXX */
	vn.v_specinfo = &si;			/* XXX */
	vn.v_rdev = dev;			/* XXX */
	error = uvm_mmap(&p->p_vmspace->vm_map, (vaddr_t *)addrp,
			 (vsize_t)len, VM_PROT_READ|VM_PROT_WRITE,
			 VM_PROT_READ|VM_PROT_WRITE,
			 flags, (void *)&vn, 0,
			 p->p_rlimit[RLIMIT_MEMLOCK].rlim_cur);
	if (error == 0)
		(void) (*gp->g_sw->gd_mode)(gp, GM_MAP, *addrp);
	return error;
}

int
grfunmap(dev_t dev, void *addr, struct proc *p)
{
	struct grf_softc *sc = grf_cd.cd_devs[GRFUNIT(dev)];
	struct grf_data *gp = sc->sc_data;
	vsize_t size;

#ifdef DEBUG
	if (grfdebug & GDB_MMAP)
		printf("grfunmap(%d): dev %x addr %p\n", p->p_pid, dev, addr);
#endif
	if (addr == 0)
		return EINVAL;		/* XXX: how do we deal with this? */
	(void) (*gp->g_sw->gd_mode)(gp, GM_UNMAP, 0);
	size = round_page(gp->g_display.gd_regsize + gp->g_display.gd_fbsize);
	uvm_unmap(&p->p_vmspace->vm_map, (vaddr_t)addr, (vaddr_t)addr + size);
	return 0;
}
