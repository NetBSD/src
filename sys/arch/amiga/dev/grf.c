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
 * Graphics display driver for the AMIGA
 * This is the hardware-independent portion of the driver.
 * Hardware access is through the grfdev routines below.
 */

#include "grf.h"
#if NGRF > 0

#include "param.h"
#include "proc.h"
#include "ioctl.h"
#include "file.h"
#include "malloc.h"

#include "device.h"
#include "grfioctl.h"
#include "grfvar.h"

#include "machine/cpu.h"

#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "vm/vm_pager.h"

#include "specdev.h"
#include "vnode.h"
#include "mman.h"

#include "ite.h"
#if NITE == 0
#define	iteon(u,f)
#define	iteoff(u,f)
#endif

int	grfprobe();
int	cc_init(), cc_mode();
int	tg_init(), tg_mode();

struct grfdev grfdev[] = {
	MANUF_BUILTIN,		PROD_BUILTIN_DISPLAY,
		cc_init,  cc_mode,  "custom chips",
	MANUF_UNILOWELL,	PROD_UNILOWELL_A2410,
		tg_init,  tg_mode,    "A2410 TIGA board",
};
int	ngrfdev = sizeof(grfdev) / sizeof(grfdev[0]);

struct	driver grfdriver = { grfprobe, "grf" };
struct	grf_softc grf_softc[NGRF];

#ifdef DEBUG
int grfdebug = 0;
#define GDB_DEVNO	0x01
#define GDB_MMAP	0x02
#define GDB_IOMAP	0x04
#define GDB_LOCK	0x08
#endif

/*
 * XXX: called from ite console init routine.
 * Does just what configure will do later but without printing anything.
 */
grfconfig()
{
	register caddr_t addr;
	register struct amiga_hw *hw;
	register struct amiga_device *ad, *nad;

	for (hw = sc_table; hw->hw_type; hw++) {
	        if (!HW_ISDEV(hw, D_BITMAP))
			continue;
		/*
		 * Found one, now match up with a logical unit number
		 */
		nad = NULL;		
		addr = hw->hw_kva;
		for (ad = amiga_dinit; ad->amiga_driver; ad++) {
			if (ad->amiga_driver != &grfdriver || ad->amiga_alive)
				continue;
			/*
			 * Wildcarded.  If first, remember as possible match.
			 */
			if (ad->amiga_addr == NULL) {
				if (nad == NULL)
					nad = ad;
				continue;
			}
			/*
			 * Not wildcarded.
			 * If exact match done searching, else keep looking.
			 */
			if (((hw->hw_manufacturer << 16) | hw->hw_product) 
			    == ad->amiga_addr) {
				nad = ad;
				break;
			}
		}
		/*
		 * Found a match, initialize
		 */
		if (nad && grfinit (nad))
			nad->amiga_addr = addr;
	}
}

/*
 * Normal init routine called by configure() code
 */
grfprobe(ad)
	struct amiga_device *ad;
{
	struct grf_softc *gp = &grf_softc[ad->amiga_unit];

	if ((gp->g_flags & GF_ALIVE) == 0 &&
	    !grfinit (ad))
		return(0);
	printf("grf%d: %d x %d ", ad->amiga_unit,
	       gp->g_display.gd_dwidth, gp->g_display.gd_dheight);
	if (gp->g_display.gd_colors == 2)
		printf("monochrome");
	else
		printf("%d color", gp->g_display.gd_colors);
	printf(" %s display\n", grfdev[gp->g_type].gd_desc);
	return(1);
}

grfinit(ad)
	struct amiga_device *ad;
{
	struct grf_softc *gp = &grf_softc[ad->amiga_unit];
	register struct grfdev *gd;

	for (gd = grfdev; gd < &grfdev[ngrfdev]; gd++)
	  if (((gd->gd_manuf << 16) | gd->gd_prod) == ad->amiga_addr)
	    break;

	if (gd < &grfdev[ngrfdev] && (*gd->gd_init)(gp, ad)) {
		gp->g_type = gd - grfdev;
		gp->g_flags = GF_ALIVE;
		return(1);
	}
	return(0);
}

/*ARGSUSED*/
grfopen(dev, flags)
	dev_t dev;
{
	int unit = GRFUNIT(dev);
	register struct grf_softc *gp = &grf_softc[unit];
	int error = 0;

	if (unit >= NGRF || (gp->g_flags & GF_ALIVE) == 0)
		return(ENXIO);
	if ((gp->g_flags & (GF_OPEN|GF_EXCLUDE)) == (GF_OPEN|GF_EXCLUDE))
		return(EBUSY);
	/*
	 * First open.
	 * XXX: always put in graphics mode.
	 */
	error = 0;
	if ((gp->g_flags & GF_OPEN) == 0) {
		gp->g_flags |= GF_OPEN;
		error = grfon(dev);
	}
	return(error);
}

/*ARGSUSED*/
grfclose(dev, flags)
	dev_t dev;
{
	register struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];

	(void) grfoff(dev);
	(void) grfunlock(gp);
	gp->g_flags &= GF_ALIVE;
	return(0);
}

/*ARGSUSED*/
grfioctl(dev, cmd, data, flag, p)
	dev_t dev;
	caddr_t data;
	struct proc *p;
{
	register struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];
	int error;

	error = 0;
	switch (cmd) {

	case GRFIOCGINFO:
		bcopy((caddr_t)&gp->g_display, data, sizeof(struct grfinfo));
		break;

	case GRFIOCON:
		error = grfon(dev);
		break;

	case GRFIOCOFF:
		error = grfoff(dev);
		break;

	case GRFIOCMAP:
		error = grfmmap(dev, (caddr_t *)data, p);
		break;

	case GRFIOCUNMAP:
		error = grfunmmap(dev, *(caddr_t *)data, p);
		break;
		
	case GRFIOCSINFO:
		error = grfsinfo (dev, (struct grfdyninfo *) data);
		break;
		
	default:
		error = EINVAL;
		break;

	}
	return(error);
}

/*ARGSUSED*/
grfselect(dev, rw)
	dev_t dev;
{
	if (rw == FREAD)
		return(0);
	return(1);
}

grflock(gp, block)
	register struct grf_softc *gp;
	int block;
{
	struct proc *p = curproc;		/* XXX */
	int error;
	extern char devioc[];

#ifdef DEBUG
	if (grfdebug & GDB_LOCK)
		printf("grflock(%d): dev %x flags %x lockpid %x\n",
		       p->p_pid, gp-grf_softc, gp->g_flags,
		       gp->g_lockp ? gp->g_lockp->p_pid : -1);
#endif
	if (gp->g_lockp) {
		if (gp->g_lockp == p)
			return(EBUSY);
		if (!block)
			return(EAGAIN);
		do {
			gp->g_flags |= GF_WANTED;
			if (error = tsleep((caddr_t)&gp->g_flags,
					   (PZERO+1) | PCATCH, devioc, 0))
				return (error);
		} while (gp->g_lockp);
	}
	gp->g_lockp = p;
	return(0);
}

grfunlock(gp)
	register struct grf_softc *gp;
{
#ifdef DEBUG
	if (grfdebug & GDB_LOCK)
		printf("grfunlock(%d): dev %x flags %x lockpid %d\n",
		       curproc->p_pid, gp-grf_softc, gp->g_flags,
		       gp->g_lockp ? gp->g_lockp->p_pid : -1);
#endif
	if (gp->g_lockp != curproc)
		return(EBUSY);
	if (gp->g_flags & GF_WANTED) {
		wakeup((caddr_t)&gp->g_flags); 
		gp->g_flags &= ~GF_WANTED;
	}
	gp->g_lockp = NULL;
	return(0);
}

/*ARGSUSED*/
grfmap(dev, off, prot)
	dev_t dev;
{
	return(grfaddr(&grf_softc[GRFUNIT(dev)], off));
}


grfon(dev)
	dev_t dev;
{
	int unit = GRFUNIT(dev);
	struct grf_softc *gp = &grf_softc[unit];

	/*
	 * XXX: iteoff call relies on devices being in same order
	 * as ITEs and the fact that iteoff only uses the minor part
	 * of the dev arg.
	 */
	iteoff(unit, 3);
	return((*grfdev[gp->g_type].gd_mode)
			(gp, (dev&GRFOVDEV) ? GM_GRFOVON : GM_GRFON));
}

grfoff(dev)
	dev_t dev;
{
	int unit = GRFUNIT(dev);
	struct grf_softc *gp = &grf_softc[unit];
	int error;

	(void) grfunmmap(dev, (caddr_t)0, curproc);
	error = (*grfdev[gp->g_type].gd_mode)
			(gp, (dev&GRFOVDEV) ? GM_GRFOVOFF : GM_GRFOFF);
	/* XXX: see comment for iteoff above */
	iteon(unit, 2);
	return(error);
}

grfsinfo(dev, dyninfo)
	dev_t dev;
	struct grfdyninfo *dyninfo;
{
	int unit = GRFUNIT(dev);
	struct grf_softc *gp = &grf_softc[unit];
	int error;

	error = grfdev[gp->g_type].gd_mode (gp, GM_GRFCONFIG, dyninfo);
	/* XXX: see comment for iteoff above */
	itereinit (unit);
	return(error);
}

grfaddr(gp, off)
	struct grf_softc *gp;
	register int off;
{
	register struct grfinfo *gi = &gp->g_display;

#if 0
	/* control registers */
	if (off >= 0 && off < gi->gd_regsize)
		return(((u_int)gi->gd_regaddr + off) >> PGSHIFT);

	/* frame buffer */
	if (off >= gi->gd_regsize && off < gi->gd_regsize+gi->gd_fbsize) {
		off -= gi->gd_regsize;
		return(((u_int)gi->gd_fbaddr + off) >> PGSHIFT);
	}
#endif
	/* bogus */
	return(-1);
}

grfmmap(dev, addrp, p)
	dev_t dev;
	caddr_t *addrp;
	struct proc *p;
{
	struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];
	int len, error;
	struct vnode vn;
	struct specinfo si;
	int flags;

#ifdef DEBUG
	if (grfdebug & GDB_MMAP)
		printf("grfmmap(%d): addr %x\n", p->p_pid, *addrp);
#endif
	len = gp->g_display.gd_regsize + gp->g_display.gd_fbsize;
	flags = MAP_FILE|MAP_SHARED;
	if (*addrp)
		flags |= MAP_FIXED;
	else
		*addrp = (caddr_t)0x1000000;	/* XXX */
	vn.v_type = VCHR;			/* XXX */
	vn.v_specinfo = &si;			/* XXX */
	vn.v_rdev = dev;			/* XXX */
	error = vm_mmap(&p->p_vmspace->vm_map, (vm_offset_t *)addrp,
			(vm_size_t)len, VM_PROT_ALL, flags, (caddr_t)&vn, 0);
	return(error);
}

grfunmmap(dev, addr, p)
	dev_t dev;
	caddr_t addr;
	struct proc *p;
{
	struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];
	vm_size_t size;
	int rv;

#ifdef DEBUG
	if (grfdebug & GDB_MMAP)
		printf("grfunmmap(%d): dev %x addr %x\n", p->p_pid, dev, addr);
#endif
	if (addr == 0)
		return(EINVAL);		/* XXX: how do we deal with this? */
	size = round_page(gp->g_display.gd_regsize + gp->g_display.gd_fbsize);
	rv = vm_deallocate(p->p_vmspace->vm_map, (vm_offset_t)addr, size);
	return(rv == KERN_SUCCESS ? 0 : EINVAL);
}


#endif	/* NGRF > 0 */
