/*	$NetBSD: cgsix_sbus.c,v 1.6 2000/06/29 07:37:54 mrg Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * color display (cgsix) driver; Sbus bus front-end.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <machine/fbio.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#ifdef DEBUG
#include <sys/proc.h>
#include <sys/syslog.h>
#endif

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/fbvar.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/conf.h>

#include <dev/sbus/sbusvar.h>

#include <sparc64/dev/btreg.h>
#include <sparc64/dev/btvar.h>
#include <sparc64/dev/cgsixreg.h>
#include <sparc64/dev/cgsixvar.h>


/* autoconfiguration driver */
static int	cgsixmatch __P((struct device *, struct cfdata *, void *));
static void	cgsixattach __P((struct device *, struct device *, void *));

struct cfattach cgsix_ca = {
	sizeof(struct cgsix_softc), cgsixmatch, cgsixattach
};


extern int fbnode;

/*
 * Match a cgsix.
 */
int
cgsixmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0);
}


/*
 * Attach a display.
 */
void
cgsixattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cgsix_softc *sc = (struct cgsix_softc *)self;
	struct sbus_attach_args *sa = aux;
	struct fbdevice *fb = &sc->sc_fb;
	int node, isconsole;
	char *name;
	bus_space_handle_t bh;

	/* Remember cookies for cgsix_mmap() */
	sc->sc_bustag = sa->sa_bustag;
	sc->sc_paddr = ((int64_t)sa->sa_slot<<32)|sa->sa_offset;

	node = sa->sa_node;

	fb->fb_device = &sc->sc_dev;
	fb->fb_type.fb_type = FBTYPE_SUNFAST_COLOR;
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags & FB_USERMASK;
	fb->fb_type.fb_depth = 8;

	fb_setsize(fb, fb->fb_type.fb_depth, 1152, 900, node);

	/*
	 * Dunno what the PROM has mapped, though obviously it must have
	 * the video RAM mapped.  Just map what we care about for ourselves
	 * (the FHC, THC, and Brooktree registers).
	 */
	if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset + CGSIX_BT_OFFSET,
			 sizeof(*sc->sc_bt),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("%s: cannot map brooktree registers\n", self->dv_xname);
		return;
	}
	sc->sc_bt = (struct bt_regs *)(u_long)bh;

	if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset + CGSIX_FHC_OFFSET,
			 sizeof(*sc->sc_fhc),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("%s: cannot map FHC registers\n", self->dv_xname);
		return;
	}
	sc->sc_fhc = (int *)(u_long)bh;

	if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset + CGSIX_THC_OFFSET,
			 sizeof(*sc->sc_thc),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("%s: cannot map THC registers\n", self->dv_xname);
		return;
	}
	sc->sc_thc = (struct cg6_thc *)(u_long)bh;

	if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset + CGSIX_TEC_OFFSET,
			 sizeof(*sc->sc_tec),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("%s: cannot map TEC registers\n", self->dv_xname);
		return;
	}
	sc->sc_tec = (struct cg6_tec_xxx *)(u_long)bh;

	sbus_establish(&sc->sc_sd, &sc->sc_dev);
	name = getpropstring(node, "model");

	isconsole = (node == fbnode);
	if (isconsole && cgsix_use_rasterconsole) {
		int ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset + CGSIX_RAM_OFFSET,
				 ramsize,
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s: cannot map pixels\n", self->dv_xname);
			return;
		}
		sc->sc_fb.fb_pixels = (caddr_t)(u_long)bh;
	}

	cg6attach(sc, name, isconsole, node == fbnode);
}
