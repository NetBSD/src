/*	$NetBSD: grf_machdep.c,v 1.17 2003/07/15 01:44:51 lukem Exp $	*/

/*
 * Copyright (c) 1991 University of Utah.
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
 * from: Utah $Hdr: grf_machdep.c 1.1 92/01/21
 *
 *	@(#)grf_machdep.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Graphics display driver for the HP300/400 DIO/DIO-II based machines.
 * This is the hardware-dependent configuration portion of the driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf_machdep.c,v 1.17 2003/07/15 01:44:51 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/grfioctl.h>
#include <x68k/dev/grfvar.h>
#include <x68k/x68k/iodevice.h>

/*
 * false when initing for the console.
 */
extern int x68k_realconfig;
int x68k_config_found __P((struct cfdata *, struct device *,
			   void *, cfprint_t));

int grfbusprint __P((void *auxp, const char *));
int grfbusmatch __P((struct device *, struct cfdata *, void *));
void grfbusattach __P((struct device *, struct device *, void *));
int grfbussearch __P((struct device *, struct cfdata *, void *));

void grfattach __P((struct device *, struct device *, void *));
int grfmatch __P((struct device *, struct cfdata *, void *));
int grfprint __P((void *, const char *));

void grfconfig __P((struct device *));
int grfinit __P((void *, int));

CFATTACH_DECL(grfbus, sizeof(struct device),
    grfbusmatch, grfbusattach, NULL, NULL);

CFATTACH_DECL(grf, sizeof(struct grf_softc),
    grfmatch, grfattach, NULL, NULL);

/*
 * only used in console init.
 */
static struct cfdata *cfdata_gbus  = NULL;
static struct cfdata *cfdata_grf   = NULL;

extern struct cfdriver grfbus_cd;

int
grfbusmatch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{
	if (strcmp(auxp, grfbus_cd.cd_name))
		return(0);

	if((x68k_realconfig == 0) || (cfdata_gbus == NULL)) {
		/*
		 * Probe layers we depend on
		 */
		if(x68k_realconfig == 0) {
			cfdata_gbus = cfp;
		}
	}
	return(1);	/* Always there	*/
}

void
grfbusattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	int i;

	if (dp == NULL) {
		i = 0;
		x68k_config_found(cfdata_gbus, NULL, (void*)&i, grfbusprint);
	} else {
		printf("\n");
		config_search(grfbussearch, dp, NULL);
	}
}

int
grfbussearch(dp, match, aux)
	struct device *dp;
	struct cfdata *match;
	
	void *aux;
{
	int i = 0;
	config_found(dp, (void*)&i, grfbusprint);
	return (0);
}

int
grfbusprint(auxp, name)
	void *auxp;
	const char *name;
{
	if(name == NULL)
		return(UNCONF);
	return(QUIET);
}

static struct grf_softc	congrf;
/*
 * XXX called from ite console init routine.
 * Does just what configure will do later but without printing anything.
 */
void
grfconfig(dp)
	struct device *dp;
{
	int unit;
	if (!dp)
		dp = (void *)&congrf;
	unit = dp->dv_unit;

	grfinit(dp, unit); /* XXX */
}

/*
 * Normal init routine called by configure() code
 */
int
grfmatch(parent, cfp, aux)
	struct device *parent;
	struct cfdata *cfp;
	void *aux;
{
	/* XXX console at grf0 */
	if (x68k_realconfig == 0) {
		if (cfp->cf_unit != 0)
			return(0);
		cfdata_grf = cfp;
	}
	return(1);
}

void
grfattach(parent, dp, aux)
	struct device *parent;
	struct device *dp;
	void *aux;
{
/*	static struct grf_softc	congrf;*/
	struct grf_softc *gp;

	grfconfig(dp);
	/*
	 * Handle exeption case: early console init
	 */
	if(dp == NULL) {
		/* Attach console ite */
		x68k_config_found(cfdata_grf, NULL, &congrf, grfprint);
		return;
	}
	gp = (struct grf_softc *)dp;
	printf(": %d x %d ", gp->g_display.gd_dwidth,
	    gp->g_display.gd_dheight);
	if (gp->g_display.gd_colors == 2)
		printf("monochrome");
	else
		printf("%d colors", gp->g_display.gd_colors);
	printf(" %s display\n", gp->g_sw->gd_desc);

	/*
	 * try and attach an ite
	 */
	config_found(dp, gp, grfprint);
}

int
grfprint(auxp, pnp)
void *auxp;
const char *pnp;
{
	if(pnp)
		aprint_normal("ite at %s", pnp);
	return(UNCONF);
}

int
grfinit(dp, unit)
	void *dp;
	int unit;
{
	struct grf_softc *gp = dp;
	register struct grfsw *gsw;
	caddr_t addr;

	if (unit == 0)
		addr = (caddr_t)IODEVbase->tvram;
	else
		addr = (caddr_t)IODEVbase->gvram;

	gsw = &grfsw[unit];
	if (gsw < &grfsw[ngrfsw] && (*gsw->gd_init)(gp, addr)) {
		gp->g_sw = gsw;
		gp->g_display.gd_id = gsw->gd_swid;
		gp->g_flags = GF_ALIVE;
		return(1);
	}
	return(0);
}
