/*	$NetBSD: grf_cc.c,v 1.39.8.1 2012/04/17 00:06:01 yamt Exp $ */

/*
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf_cc.c,v 1.39.8.1 2012/04/17 00:06:01 yamt Exp $");

#include "grfcc.h"
#include "ite.h"
#if NGRFCC > 0
/*
 * currently this is a backward compat hack that interface to
 * view.c
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <machine/cpu.h>
#include <amiga/amiga/color.h>	/* DEBUG */
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/dev/grfioctl.h>
#include <amiga/dev/grfvar.h>
#include <amiga/dev/grf_ccreg.h>
#include <amiga/dev/grfabs_reg.h>
#include <amiga/dev/viewioctl.h>

#include "view.h"

int grfccmatch(struct device *, struct cfdata *, void *);
int grfccprint(void *, const char *);
void grfccattach(struct device *, struct device *, void *);
void grf_cc_on(struct grf_softc *);

CFATTACH_DECL(grfcc, sizeof(struct grf_softc),
    grfccmatch, grfccattach, NULL, NULL);

/*
 * only used in console init
 */
static struct cfdata *cfdata;

/*
 * we make sure to only init things once.  this is somewhat
 * tricky regarding the console.
 */
int
grfccmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	static int ccconunit = -1;
	char *mainbus_name = auxp;
	extern const struct cdevsw view_cdevsw;

	/*
	 * allow only one cc console
	 */
	if (amiga_realconfig == 0 && ccconunit != -1)
		return(0);
	if (matchname("grfcc", mainbus_name) == 0)
		return(0);
	if (amiga_realconfig == 0 || ccconunit != cfp->cf_unit) {
		if (grfcc_probe() == 0)
			return(0);
		viewprobe();
		/*
		 * XXX nasty hack. opens view[0] and never closes.
		 */
		if ((*view_cdevsw.d_open)(0, 0, 0, NULL))
			return(0);
		if (amiga_realconfig == 0) {
			ccconunit = cfp->cf_unit;
			cfdata = cfp;
		}
	}
	return(1);
}

/*
 * attach to the grfbus (mainbus)
 */
void
grfccattach(struct device *pdp, struct device *dp, void *auxp)
{
	static struct grf_softc congrf;
	struct grf_softc *gp;

	if (dp == NULL)
		gp = &congrf;
	else
		gp = (struct grf_softc *)dp;

	if (dp != NULL && congrf.g_regkva != 0) {
		/*
		 * we inited earlier just copy the info
		 * take care not to copy the device struct though.
		 */
		memcpy(&gp->g_display, &congrf.g_display,
		    (char *)&gp[1] - (char *)&gp->g_display);
	} else {
		gp->g_unit = GRF_CC_UNIT;
		gp->g_flags = GF_ALIVE;
		gp->g_mode = cc_mode;
#if NITE > 0
		gp->g_conpri = grfcc_cnprobe();
		grfcc_iteinit(gp);
#endif
		grf_cc_on(gp);
	}
	if (dp != NULL)
		printf("\n");
	/*
	 * attach grf
	 */
	amiga_config_found(cfdata, &gp->g_device, gp, grfccprint);
}

int
grfccprint(void *auxp, const char *pnp)
{
	if (pnp)
		aprint_normal("grf%d at %s", ((struct grf_softc *)auxp)->g_unit,
			pnp);
	return(UNCONF);
}

/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 */
/*ARGSUSED*/
int
cc_mode(struct grf_softc *gp, u_long cmd, void *arg, u_long a2, int a3)
{
	extern const struct cdevsw view_cdevsw;

	switch (cmd) {
	case GM_GRFON:
		grf_cc_on(gp);
		return(0);
	case GM_GRFOFF:
		(*view_cdevsw.d_ioctl)(0, VIOCREMOVE, NULL, -1, NULL);
		return(0);
	case GM_GRFCONFIG:
	default:
		break;
	}
	return(EPASSTHROUGH);
}

void
grf_cc_on(struct grf_softc *gp)
{
	struct view_size vs;
	bmap_t bm;
	struct grfinfo *gi;
	extern const struct cdevsw view_cdevsw;

	gi = &gp->g_display;

	/* XXX type of bm ? */
	(*view_cdevsw.d_ioctl)(0, VIOCGBMAP, (void *)&bm, -1, NULL);

	gp->g_data = (void *) 0xDeadBeaf; /* not particularly clean.. */

	gi->gd_regaddr = (void *) 0xdff000;	/* depricated */
	gi->gd_regsize = round_page(sizeof (custom));
	gi->gd_fbaddr  = bm.hardware_address;
	gi->gd_fbsize  = bm.depth*bm.bytes_per_row*bm.rows;

	if ((*view_cdevsw.d_ioctl)(0, VIOCGSIZE, (void *)&vs, -1, NULL)) {
		/* XXX type of vs ? */
		/* fill in some default values... XXX */
		vs.width = 640;
		vs.height = 400;
		vs.depth = 2;
	}
	gi->gd_colors = 1 << vs.depth;
	gi->gd_planes = vs.depth;

	gi->gd_fbwidth = vs.width;
	gi->gd_fbheight = vs.height;
	gi->gd_fbx = 0;
	gi->gd_fby = 0;
	gi->gd_dwidth = vs.width;
	gi->gd_dheight = vs.height;
	gi->gd_dx = 0;
	gi->gd_dy = 0;

	gp->g_regkva = (void *)0xDeadBeaf;	/* builtin */
	gp->g_fbkva = NULL;		/* not needed, view internal */

	(*view_cdevsw.d_ioctl)(0, VIOCDISPLAY, NULL, -1, NULL);
}
#endif

