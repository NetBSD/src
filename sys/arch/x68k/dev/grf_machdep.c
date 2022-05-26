/*	$NetBSD: grf_machdep.c,v 1.35 2022/05/26 14:33:29 tsutsui Exp $	*/

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
 * from: Utah $Hdr: grf_machdep.c 1.1 92/01/21
 *
 *	@(#)grf_machdep.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Graphics display driver for the HP300/400 DIO/DIO-II based machines.
 * This is the hardware-dependent configuration portion of the driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf_machdep.c,v 1.35 2022/05/26 14:33:29 tsutsui Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/grfioctl.h>
#include <x68k/dev/grfvar.h>
#include <x68k/x68k/iodevice.h>

#include "ioconf.h"

/* grfbus: is this necessary? */
int grfbusprint(void *, const char *);
int grfbusmatch(device_t, cfdata_t, void *);
void grfbusattach(device_t, device_t, void *);
int grfbussearch(device_t, cfdata_t, const int *, void *);

/* grf itself */
int grfmatch(device_t, cfdata_t, void *);
void grfattach(device_t, device_t, void *);
int grfprint(void *, const char *);

static int grfinit(struct grf_softc *, int);

CFATTACH_DECL_NEW(grfbus, 0,
    grfbusmatch, grfbusattach, NULL, NULL);

CFATTACH_DECL_NEW(grf, sizeof(struct grf_softc),
    grfmatch, grfattach, NULL, NULL);

int
grfbusmatch(device_t parent, cfdata_t cf, void *aux)
{
	if (strcmp(aux, grfbus_cd.cd_name))
		return (0);

	return (1);
}

void
grfbusattach(device_t parent, device_t self, void *aux)
{

	aprint_normal("\n");
	config_search(self, NULL,
	    CFARGS(.search = grfbussearch));
}

int
grfbussearch(device_t self, cfdata_t match, const int *ldesc, void *aux)
{

	config_found(self, &match->cf_loc[GRFBCF_ADDR], grfbusprint, CFARGS_NONE);
	return (0);
}

int
grfbusprint(void *aux, const char *name)
{

	if (name == NULL)
		return (UNCONF);
	return (QUIET);
}

/*
 * Normal init routine called by configure() code
 */
int
grfmatch(device_t parent, cfdata_t cfp, void *aux)
{
	int addr;

	addr = cfp->cf_loc[GRFBCF_ADDR];
	if (addr < 0 || addr > ngrfsw)
		return 0;

	return 1;
}

struct grf_softc congrf;

void
grfattach(device_t parent, device_t self, void *aux)
{
	struct grf_softc *gp;
	struct cfdata *cf;
	int addr;

	cf = device_cfdata(self);
	addr = cf->cf_loc[GRFBCF_ADDR];

	gp = device_private(self);
	gp->g_device = self;
	gp->g_cfaddr = addr;
	grfinit(gp, addr);

	aprint_normal(": %d x %d ",
		gp->g_display.gd_dwidth, gp->g_display.gd_dheight);
	if (gp->g_display.gd_colors == 2)
		aprint_normal("monochrome");
	else
		aprint_normal("%d colors", gp->g_display.gd_colors);
	aprint_normal(" %s display\n", gp->g_sw->gd_desc);

	/*
	 * try and attach an ite
	 */
	config_found(self, gp, grfprint, CFARGS_NONE);
}

int
grfprint(void *aux, const char *pnp)
{

	if (pnp)
		aprint_normal("ite at %s", pnp);
	return UNCONF;
}

int
grfinit(struct grf_softc *gp, int cfaddr)
{
	struct grfsw *gsw;
	void *addr;

	if (cfaddr == 0)
		addr = (void *)__UNVOLATILE(IODEVbase->tvram);
	else
		addr = (void *)__UNVOLATILE(IODEVbase->gvram);

	gsw = &grfsw[cfaddr];
	if (gsw < &grfsw[ngrfsw] && (*gsw->gd_init)(gp, addr)) {
		gp->g_sw = gsw;
		gp->g_display.gd_id = gsw->gd_swid;
		gp->g_flags = GF_ALIVE;
		return 1;
	}

	return 0;
}

void
grf_config_console(void)
{
	grfinit(&congrf, 0);
}
