/*	$NetBSD: grf.c,v 1.18 1995/04/13 04:04:58 briggs Exp $	*/

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
 * Graphics display driver for the Macintosh.
 * This is the hardware-independent portion of the driver.
 * Hardware access is through the grfdev routines below.
 */

#include "grf.h"
#if NGRF > 0

#include <sys/types.h>
#include "param.h"
#include "proc.h"
#include "ioctl.h"
#include "file.h"
#include "malloc.h"

#include <sys/device.h>
#include "nubus.h"
#include <machine/grfioctl.h>
#include "grfvar.h"

#include <machine/cpu.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <vm/vm_pager.h>

#include <vnode.h>
#include <miscfs/specfs/specdev.h>
#include <mman.h>

#include "ite.h"
#if NITE == 0
#define	iteon(u,f)
#define	iteoff(u,f)
#endif

int	grfprobe();
int	macvideo_init(), macvideo_mode();

struct grfdev grfdev[] = {
	GID_MAC,	GRFMAC,	macvideo_init,	macvideo_mode, "MacVideo",
};
int	ngrfdev = sizeof(grfdev) / sizeof(grfdev[0]);

/* struct	driver grfdriver = { grfprobe, "grf" }; */
struct	grf_softc grf_softc[NGRF];

int gNumGrfDev=0;

#ifdef DEBUG
int grfdebug = 0xff;
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
#if 0
	register caddr_t addr;
	register struct nubus_hw *hw;
	register struct mac_device *hd, *nhd;
	int i;

	for (i=0,hw = nubus_table;i<NUBUS_MAXSLOTS; hw++,i++) {
	        if (hw->Slot.type!=NUBUS_VIDEO)
			continue;
		/*
		 * Found one, now match up with a logical unit number
		 */
		nhd = NULL;		
		addr = hw->addr;
		for (hd = mac_dinit; hd->mac_driver; hd++) {
			if ((hd->mac_driver != &grfdriver) || hd->mac_alive)
				continue;
nhd = hd;
nhd->mac_addr = addr;
nhd->mac_unit = gNumGrfDev++;
break;
			/*
			 * Not wildcarded.
			 * If exact match done searching, else keep looking.
			 */
			/* if (sctova(hd->mac_addr) == addr) { */
				/* nhd = hd; */
				/* break; */
			/* } */
		}
		/*
		 * Found a match, initialize
		 */
		if (nhd && grfprobe(nhd))
		{
			nhd->mac_alive=1;
			hw->claimed = 1;
		}
	}
#endif
}

/*
 * Normal init routine called by configure() code
 */
grfprobe(nu, unit)
	struct nubus_hw *nu;
	int		unit;
{
	struct grf_softc *gp = &grf_softc[unit];

	if ((gp->g_flags & GF_ALIVE) == 0 &&
	    !grfinit(nu, unit)) {
		printf("\n");
		return(0);
	}
	printf(": %d x %d ",
	       gp->g_display.gd_dwidth, gp->g_display.gd_dheight);
	if (gp->g_display.gd_colors == 2)
		printf("monochrome");
	else
		printf("%d color", gp->g_display.gd_colors);
	printf(" %s (%s) display\n", grfdev[gp->g_type].gd_desc, nu->Slot.name);
	gp->g_data = (void *)&nu->Slot;
	return(1);
}

static int
matchvideocard(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	struct nubus_hw	*nu = (struct nubus_hw *) aux;

	return (nu->Slot.type == NUBUS_VIDEO);
}

static void
grf_attach(parent, dev, aux)
	struct device	*parent, *dev;
	void		*aux;
{
	struct nubus_hw	*nu = (struct nubus_hw *) aux;

	grfprobe(nu, dev->dv_unit);
}

struct cfdriver grfcd =
      { NULL, "grf", matchvideocard, grf_attach,
	DV_DULL, sizeof(struct device), NULL, 0 };

grfinit(nu, unit)
	struct nubus_hw *nu;
{
	struct grf_softc *gp = &grf_softc[unit];
	struct grfreg *gr;
	register struct grfdev *gd;

	for (gd = grfdev; gd < &grfdev[ngrfdev]; gd++)
		/* if (gd->gd_hardid == gr->gr_id2) */
			break;
	if (gd < &grfdev[ngrfdev] && (*gd->gd_init)(gp, nu)) {
		gp->g_display.gd_id = gd->gd_softid;
		gp->g_type = gd - grfdev;
		gp->g_flags = GF_ALIVE;
		return(1);
	}
	return(0);
}

void fake_internal (void)
{
	int			i, j;
	struct grf_softc	*gp;
	struct grfinfo		*gi;
	struct grfterm		*gt;
	struct grfmouse		*gm;
	extern unsigned long	int_video_start;

	if (int_video_start == 0) {
		return;
	}

	for (i = 0; i < NGRF; i++) {
		gp = &grf_softc[i];
		if ((gp->g_flags & GF_ALIVE) == 0) {
			break;
		}
	}

	if (i == NGRF) {
		printf ("grf: not enough grf's to map internal video.\n");
		return;
	}

	gp->g_type = 0;
	gp->g_flags = GF_ALIVE;

	gi = &(gp->g_display);
	gi->gd_id = GRFMAC;
	gi->gd_regsize = 0;
	gi->gd_colors = 1;
	gi->gd_planes = 1;
	gi->gd_dwidth = gi->gd_fbwidth = 640;  /* XXX */
	gi->gd_dheight = gi->gd_fbheight = 480; /* XXX */
	gi->gd_fbsize = gi->gd_dwidth * gi->gd_dheight;
	gi->gd_fbrowbytes = 80; /* XXX Hack */
	gi->gd_fbaddr = (caddr_t)0;
	gp->g_fbkva = gi->gd_fbaddr;
}

/*ARGSUSED*/
grfopen(dev, flags)
	dev_t dev;
{
	int unit = GRFUNIT(dev);
	register struct grf_softc *gp = &grf_softc[unit];
	int error = 0;
	static int faked; /* Whether we've faked internal video yet */

	if (!faked) {
		fake_internal ();
		faked = 1;
	}

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

	/* XXX: compatibility hack */
	case OGRFIOCGINFO:
		bcopy((caddr_t)&gp->g_display, data, sizeof(struct ogrfinfo));
		break;

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
		error = grfmap(dev, (caddr_t *)data, p);
		break;

	case GRFIOCUNMAP:
		error = grfunmap(dev, *(caddr_t *)data, p);
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
grfmmap(dev, off, prot)
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

	(void) grfunmap(dev, (caddr_t)0, curproc);
	error = (*grfdev[gp->g_type].gd_mode)
			(gp, (dev&GRFOVDEV) ? GM_GRFOVOFF : GM_GRFOFF);
	/* XXX: see comment for iteoff above */
	iteon(unit, 2);
	return(error);
}

grfaddr(gp, off)
	struct grf_softc *gp;
	register int off;
{
	register struct grfinfo *gi = &gp->g_display;

	/* control registers */
	if (off >= 0 && off < gi->gd_regsize)
		return(((u_int)gi->gd_regaddr + off) >> PGSHIFT);

	/* frame buffer */
	if (off >= gi->gd_regsize && off < gi->gd_regsize+gi->gd_fbsize) {
		off -= gi->gd_regsize;
		return(((u_int)gi->gd_fbaddr + off) >> PGSHIFT);
	}
	/* bogus */
	return(-1);
}

grfmap(dev, addrp, p)
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
		printf("grfmap(%d): addr %x\n", p->p_pid, *addrp);
#endif
	len = gp->g_display.gd_regsize + gp->g_display.gd_fbsize;
	flags = MAP_SHARED;
	if (*addrp)
		flags |= MAP_FIXED;
	else
	{
		return 12;
		*addrp = (caddr_t)0x1000000;	/* XXX */
	}
	vn.v_type = VCHR;			/* XXX */
	vn.v_specinfo = &si;			/* XXX */
	vn.v_rdev = dev;			/* XXX */

	error = vm_mmap(&p->p_vmspace->vm_map, (vm_offset_t *)addrp,
		(vm_size_t)len, VM_PROT_ALL, VM_PROT_ALL, flags, (caddr_t)&vn,
		0);

	/* Offset into page: */
	*addrp += (unsigned long)gp->g_display.gd_fbaddr & 0xfff;

	return(error);
}

grfunmap(dev, addr, p)
	dev_t dev;
	caddr_t addr;
	struct proc *p;
{
	struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];
	vm_size_t size;
	int rv;

#ifdef DEBUG
	if (grfdebug & GDB_MMAP)
		printf("grfunmap(%d): dev %x addr %x\n", p->p_pid, dev, addr);
#endif
	if (addr == 0)
		return(EINVAL);		/* XXX: how do we deal with this? */
	size = round_page(gp->g_display.gd_regsize + gp->g_display.gd_fbsize);
	rv = vm_deallocate(&p->p_vmspace->vm_map, (vm_offset_t)addr, size);
	return(rv == KERN_SUCCESS ? 0 : EINVAL);
}

#endif	/* NGRF > 0 */

static char zero = 0;

static void
macvideo_intr(int unit, int slot)
{
	struct grf_softc *gp = &grf_softc[unit];

	((char *)(0xf0000000 | ((long)slot << 24)))[0xa0000] = zero;
	return 1;
}

int
macvideo_init(struct grf_softc *gp,struct nubus_hw *nu)
{
	int i=0;
	struct grfinfo *gi;
	struct imagedata *image;
	struct imagedata imageSpace;

/*
 * find out which nubus slot this guy is in, then get the video params 
 */
	image=(struct imagedata *)NUBUS_GetImageData(&(nu->Slot),&imageSpace );

	gi = &(gp->g_display);
	gi->gd_regsize=0;
	gi->gd_colors=1;
	gi->gd_planes=1;
	gi->gd_dwidth=gi->gd_fbwidth=image->right;
	gi->gd_dheight=gi->gd_fbheight=image->bottom;
	gi->gd_fbsize=image->rowbytes*image->bottom;
	gi->gd_fbrowbytes=image->rowbytes;
	gi->gd_fbaddr=(caddr_t) ((u_long)image->offset+(u_long)nu->addr);
	gp->g_fbkva=gi->gd_fbaddr;

	add_nubus_intr((unsigned int)nu->addr & 0xFF000000, macvideo_intr,
			(gp - grf_softc));

	gNumGrfDev++;
	
	return 1;
}

int
macvideo_mode(struct grf_softc *gp,int cmd, void *arg)
{
	return 0;
}
