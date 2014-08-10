/*	$NetBSD: grf.c,v 1.43.2.1 2014/08/10 06:54:10 tls Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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
 * Graphics display driver for the X68K machines.
 * This is the hardware-independent portion of the driver.
 * Hardware access is through the machine dependent grf switch routines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf.c,v 1.43.2.1 2014/08/10 06:54:10 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/mman.h>
#include <sys/conf.h>

#include <machine/cpu.h>
#include <machine/grfioctl.h>

#include <x68k/dev/grfvar.h>
#include <x68k/dev/itevar.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>

#include <miscfs/specfs/specdev.h>

#include "ite.h"
#if NITE == 0
#define	iteon(u,f)	0
#define	iteoff(u,f)
#define	ite_reinit(u)
#endif

#ifdef DEBUG
int grfdebug = 0;
#define GDB_DEVNO	0x01
#define GDB_MMAP	0x02
#define GDB_IOMAP	0x04
#define GDB_LOCK	0x08
#endif

static int grfon(struct grf_softc *);
static int grfoff(struct grf_softc *);
static off_t grfaddr(struct grf_softc *, off_t);
static int grfmap(dev_t, void **, struct proc *);
static int grfunmap(dev_t, void *, struct proc *);

extern struct cfdriver grf_cd;

dev_type_open(grfopen);
dev_type_close(grfclose);
dev_type_ioctl(grfioctl);
dev_type_mmap(grfmmap);

const struct cdevsw grf_cdevsw = {
	.d_open = grfopen,
	.d_close = grfclose,
	.d_read = nullread,
	.d_write = nullwrite,
	.d_ioctl = grfioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = grfmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/*ARGSUSED*/
int
grfopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct grf_softc *gp;
	int error = 0;

	gp = device_lookup_private(&grf_cd, GRFUNIT(dev));
	if (gp == NULL)
		return ENXIO;

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
		error = grfon(gp);
	}
	return error;
}

/*ARGSUSED*/
int
grfclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct grf_softc *gp = device_lookup_private(&grf_cd, GRFUNIT(dev));

	if ((gp->g_flags & GF_ALIVE) == 0)
		return ENXIO;

	(void) grfoff(gp);
	gp->g_flags &= GF_ALIVE;

	return 0;
}

/*ARGSUSED*/
int
grfioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int unit = GRFUNIT(dev);
	struct grf_softc *gp = device_lookup_private(&grf_cd, GRFUNIT(dev));
	int error;

	if ((gp->g_flags & GF_ALIVE) == 0)
		return ENXIO;

	error = 0;
	switch (cmd) {

	case GRFIOCGINFO:
		memcpy(data, (void *)&gp->g_display, sizeof(struct grfinfo));
		break;

	case GRFIOCON:
		error = grfon(gp);
		break;

	case GRFIOCOFF:
		error = grfoff(gp);
		break;

	case GRFIOCMAP:
		error = grfmap(dev, (void **)data, l->l_proc);
		break;

	case GRFIOCUNMAP:
		error = grfunmap(dev, *(void **)data, l->l_proc);
		break;

	case GRFSETVMODE:
		error = (*gp->g_sw->gd_mode)(gp, GM_GRFSETVMODE, data);
		if (error == 0)
			ite_reinit(unit);
		break;

	default:
		error = EINVAL;
		break;

	}
	return error;
}

/*ARGSUSED*/
paddr_t
grfmmap(dev_t dev, off_t off, int prot)
{

	return grfaddr(device_lookup_private(&grf_cd, GRFUNIT(dev)), off);
}

int
grfon(struct grf_softc *gp)
{
	int unit = device_unit(gp->g_device);

	/*
	 * XXX: iteoff call relies on devices being in same order
	 * as ITEs and the fact that iteoff only uses the minor part
	 * of the dev arg.
	 */
	iteoff(unit, 2);

	return (*gp->g_sw->gd_mode)(gp, GM_GRFON, (void *) 0);
}

int
grfoff(struct grf_softc *gp)
{
	int unit = device_unit(gp->g_device);
	int error;

#if 0				/* always fails in EINVAL... */
	(void) grfunmap(dev, (void *) 0, curproc);
#endif
	error = (*gp->g_sw->gd_mode)(gp, GM_GRFOFF, (void *) 0);
	/* XXX: see comment for iteoff above */
	iteon(unit, 2);

	return error;
}

off_t
grfaddr(struct grf_softc *gp, off_t off)
{
	struct grfinfo *gi = &gp->g_display;

	/* control registers */
	if (off >= 0 && off < gi->gd_regsize)
		return ((u_int)gi->gd_regaddr + off) >> PGSHIFT;

	/* frame buffer */
	if (off >= gi->gd_regsize && off < gi->gd_regsize+gi->gd_fbsize) {
		off -= gi->gd_regsize;
		return ((u_int)gi->gd_fbaddr + off) >> PGSHIFT;
	}
	/* bogus */
	return -1;
}

int
grfmap(dev_t dev, void **addrp, struct proc *p)
{
	struct grf_softc *gp = device_lookup_private(&grf_cd, GRFUNIT(dev));
	int len, error;
	struct vnode vn;
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
		*addrp = (void *)p->p_emul->e_vm_default_addr(p, 
		    (vaddr_t)p->p_vmspace->vm_daddr, len);

	vn.v_type = VCHR;			/* XXX */
	vn.v_rdev = dev;			/* XXX */
	error = uvm_mmap(&p->p_vmspace->vm_map, (vaddr_t *)addrp,
			 (vsize_t)len, VM_PROT_ALL, VM_PROT_ALL,
			 flags, (void *)&vn, 0,
			 p->p_rlimit[RLIMIT_MEMLOCK].rlim_cur);
	if (error == 0)
		(void) (*gp->g_sw->gd_mode)(gp, GM_MAP, *addrp);

	return error;
}

int
grfunmap(dev_t dev, void *addr, struct proc *p)
{
	struct grf_softc *gp = device_lookup_private(&grf_cd, GRFUNIT(dev));
	vsize_t size;

#ifdef DEBUG
	if (grfdebug & GDB_MMAP) {
		printf("grfunmap(%d): dev %x addr %p\n",
			p->p_pid, GRFUNIT(dev), addr);
	}
#endif
	if (addr == 0)
		return EINVAL;		/* XXX: how do we deal with this? */
	(void) (*gp->g_sw->gd_mode)(gp, GM_UNMAP, 0);
	size = round_page(gp->g_display.gd_regsize + gp->g_display.gd_fbsize);
	uvm_unmap(&p->p_vmspace->vm_map, (vaddr_t)addr,
	    (vaddr_t)addr + size);

	return 0;
}
