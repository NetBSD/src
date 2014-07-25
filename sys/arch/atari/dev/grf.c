/*	$NetBSD: grf.c,v 1.49 2014/07/25 08:10:32 dholland Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman
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
 * from: Utah $Hdr: grf.c 1.31 91/01/21$
 *
 *	@(#)grf.c	7.8 (Berkeley) 5/7/91
 */

/*
 * Graphics display driver for the Atari
 * This is the hardware-independent portion of the driver.
 * Hardware access is through the grf_softc->g_mode routine.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf.c,v 1.49 2014/07/25 08:10:32 dholland Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mman.h>

#include <machine/cpu.h>

#include <uvm/uvm_extern.h>

#include <atari/atari/device.h>
#include <atari/dev/grfioctl.h>
#include <atari/dev/grfabs_reg.h>
#include <atari/dev/grfvar.h>
#include <atari/dev/itevar.h>
#include <atari/dev/viewioctl.h>
#include <atari/dev/viewvar.h>

#include "ioconf.h"

#include "grfcc.h"
#include "grfet.h"
#define	NGRF	(NGRFCC + NGRFET)

#if NGRF > 0

#include "ite.h"
#if NITE == 0
#define	ite_on(u,f)
#define	ite_off(u,f)
#define ite_reinit(d)
#endif

int grfon(dev_t);
int grfoff(dev_t);
int grfsinfo(dev_t, struct grfdyninfo *);

int grfbusprint(void *, const char *);
int grfbusmatch(device_t, cfdata_t, void *);
void grfbusattach(device_t, device_t, void *);

/*
 * pointers to grf drivers device structs 
 */
struct grf_softc *grfsp[NGRF]; /* XXX */

CFATTACH_DECL_NEW(grfbus, 0,
    grfbusmatch, grfbusattach, NULL, NULL);

dev_type_open(grfopen);
dev_type_close(grfclose);
dev_type_ioctl(grfioctl);
dev_type_mmap(grfmmap);

const struct cdevsw grf_cdevsw = {
	.d_open = grfopen,
	.d_close = grfclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = grfioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = grfmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/*
 * only used in console init.
 */
static cfdata_t cfdata_gbus  = NULL;

int
grfbusmatch(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp(aux, grfbus_cd.cd_name))
		return 0;

	if (atari_realconfig == 0)
		cfdata_gbus = cf;
	return 1;	/* Always there	*/
}

void
grfbusattach(device_t parent, device_t self, void *aux)
{
	grf_auxp_t	grf_auxp;

	grf_auxp.busprint       = grfbusprint;
	grf_auxp.from_bus_match = 1;

	if (self == NULL) /* Console init	*/
		atari_config_found(cfdata_gbus, NULL, &grf_auxp, grfbusprint);
	else {
		printf("\n");
		config_found(self, &grf_auxp, grfbusprint);
	}
}

int
grfbusprint(void *aux, const char *name)
{

	if (name == NULL)
		return UNCONF;
	return QUIET;
}

/*ARGSUSED*/
int
grfopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct grf_softc *gp;

	if (GRFUNIT(dev) >= NGRF)
		return ENXIO;

	gp = grfsp[GRFUNIT(dev)];
	if (gp == NULL)
		return ENXIO;

	if ((gp->g_flags & GF_ALIVE) == 0)
		return ENXIO;

	if ((gp->g_flags & (GF_OPEN|GF_EXCLUDE)) == (GF_OPEN|GF_EXCLUDE))
		return EBUSY;
	grf_viewsync(gp);

	return 0;
}

/*ARGSUSED*/
int
grfclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct grf_softc *gp;

	gp = grfsp[GRFUNIT(dev)];
	(void)grfoff(dev);
	gp->g_flags &= GF_ALIVE;
	return 0;
}

/*ARGSUSED*/
int
grfioctl(dev_t dev, u_long cmd, void * data, int flag, struct lwp *l)
{
	struct grf_softc	*gp;
	int			error;
	extern const struct cdevsw view_cdevsw;

	gp = grfsp[GRFUNIT(dev)];
	error = 0;

	switch (cmd) {
	case OGRFIOCGINFO:
	        /* argl.. no bank-member.. */
	  	memcpy(data, (void *)&gp->g_display, sizeof(struct grfinfo)-4);
		break;
	case GRFIOCGINFO:
		memcpy(data, (void *)&gp->g_display, sizeof(struct grfinfo));
		break;
	case GRFIOCON:
		error = grfon(dev);
		break;
	case GRFIOCOFF:
		error = grfoff(dev);
		break;
	case GRFIOCSINFO:
		error = grfsinfo(dev, (struct grfdyninfo *) data);
		break;
	case GRFGETVMODE:
		return gp->g_mode(gp, GM_GRFGETVMODE, data, 0, 0);
	case GRFSETVMODE:
		error = gp->g_mode(gp, GM_GRFSETVMODE, data, 0, 0);
		if (error == 0 && gp->g_itedev)
			ite_reinit(gp->g_itedev);
		break;
	case GRFGETNUMVM:
		return gp->g_mode(gp, GM_GRFGETNUMVM, data, 0, 0);
	/*
	 * these are all hardware dependent, and have to be resolved
	 * in the respective driver.
	 */
	case GRFIOCPUTCMAP:
	case GRFIOCGETCMAP:
	case GRFIOCSSPRITEPOS:
	case GRFIOCGSPRITEPOS:
	case GRFIOCSSPRITEINF:
	case GRFIOCGSPRITEINF:
	case GRFIOCGSPRITEMAX:
	default:
		/*
		 * check to see whether it's a command recognized by the
		 * view code.
		 */
		return (*view_cdevsw.d_ioctl)(gp->g_viewdev,
		    cmd, data, flag, l);
		error = EINVAL;
		break;

	}
	return error;
}

/*
 * map the contents of a graphics display card into process' 
 * memory space.
 */
paddr_t
grfmmap(dev_t dev, off_t off, int prot)
{
	struct grf_softc	*gp;
	struct grfinfo		*gi;
	u_int			vgabase, linbase;
	
	gp = grfsp[GRFUNIT(dev)];
	gi = &gp->g_display;

	vgabase = gi->gd_vgabase;
	linbase = gi->gd_linbase;

	/* 
	 * control registers
	 */
	if (off >= 0 && off < gi->gd_regsize)
		return ((paddr_t)gi->gd_regaddr + off) >> PGSHIFT;

	/*
	 * VGA memory
	 */
	if (off >= vgabase && off < (vgabase + gi->gd_vgasize))
		return ((paddr_t)gi->gd_vgaaddr - vgabase + off) >> PGSHIFT;

	/*
	 * frame buffer
	 */
	if (off >= linbase && off < (linbase + gi->gd_fbsize))
		return ((paddr_t)gi->gd_fbaddr - linbase + off) >> PGSHIFT;
	return -1;
}

int
grfon(dev_t dev)
{
	struct grf_softc *gp;

	gp = grfsp[GRFUNIT(dev)];

	if (gp->g_flags & GF_GRFON)
		return 0;

	gp->g_flags |= GF_GRFON;
	if (gp->g_itedev != NODEV)
		ite_off(gp->g_itedev, 3);

	return gp->g_mode(gp, (dev & GRFOVDEV) ? GM_GRFOVON : GM_GRFON,
	    NULL, 0, 0);
}

int
grfoff(dev_t dev)
{
	struct grf_softc *gp;
	int error;

	gp = grfsp[GRFUNIT(dev)];

	if ((gp->g_flags & GF_GRFON) == 0)
		return 0;

	gp->g_flags &= ~GF_GRFON;
	error = gp->g_mode(gp, (dev & GRFOVDEV) ? GM_GRFOVOFF : GM_GRFOFF,
	    NULL, 0, 0);

	/*
	 * Closely tied together no X's
	 */
	if (gp->g_itedev != NODEV)
		ite_on(gp->g_itedev, 2);

	return error;
}

int
grfsinfo(dev_t dev, struct grfdyninfo *dyninfo)
{
	struct grf_softc *gp;
	int error;

	gp = grfsp[GRFUNIT(dev)];
	error = gp->g_mode(gp, GM_GRFCONFIG, dyninfo, 0, 0);

	/*
	 * Closely tied together no X's
	 */
	if (gp->g_itedev != NODEV)
		ite_reinit(gp->g_itedev);
	return error;
}

/*
 * Get the grf-info in sync with underlying view.
 */
void
grf_viewsync(struct grf_softc *gp)
{
	struct view_size	vs;
	bmap_t			bm;
	struct grfinfo		*gi;
	extern const struct cdevsw view_cdevsw;

	gi = &gp->g_display;

	(*view_cdevsw.d_ioctl)(gp->g_viewdev, VIOCGBMAP, (void *)&bm,
			       0, NOLWP);
  
	gp->g_data = (void *) 0xDeadBeaf; /* not particularly clean.. */
  
	gi->gd_fbaddr  = bm.hw_address;
	gi->gd_fbsize  = bm.phys_mappable;
	gi->gd_linbase = bm.lin_base;
	gi->gd_regaddr = bm.hw_regs;
	gi->gd_regsize = bm.reg_size;
	gi->gd_vgaaddr = bm.vga_address;
	gi->gd_vgasize = bm.vga_mappable;
	gi->gd_vgabase = bm.vga_base;

	if ((*view_cdevsw.d_ioctl)(gp->g_viewdev, VIOCGSIZE, (void *)&vs, 0,
	    NOLWP)) {
		/*
		 * fill in some default values...
		 * XXX: Should _never_ happen
		 */
		vs.width  = 640;
		vs.height = 400;
		vs.depth  = 1;
	}
	gi->gd_colors = 1 << vs.depth;
	gi->gd_planes = vs.depth;
  
	gi->gd_fbwidth         = vs.width;
	gi->gd_fbheight        = vs.height;
	gi->gd_dyn.gdi_fbx     = 0;
	gi->gd_dyn.gdi_fby     = 0;
	gi->gd_dyn.gdi_dwidth  = vs.width;
	gi->gd_dyn.gdi_dheight = vs.height;
	gi->gd_dyn.gdi_dx      = 0;
	gi->gd_dyn.gdi_dy      = 0;
}    

/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 */
/*ARGSUSED*/
int
grf_mode(struct grf_softc *gp, int cmd, void *arg, int a2, int a3)
{
	extern const struct cdevsw view_cdevsw;

	switch (cmd) {
		case GM_GRFON:
			/*
			 * Get in sync with view, ite might have changed it.
			 */
			grf_viewsync(gp);
			(*view_cdevsw.d_ioctl)(gp->g_viewdev, VIOCDISPLAY,
					       NULL, 0, NOLWP);
			return 0;
	case GM_GRFOFF:
			(*view_cdevsw.d_ioctl)(gp->g_viewdev, VIOCREMOVE,
					       NULL, 0, NOLWP);
			return 0;
	case GM_GRFCONFIG:
	default:
			break;
	}
	return EPASSTHROUGH;
}
#endif	/* NGRF > 0 */
