/*	$NetBSD: cgsix_obio.c,v 1.6 2000/06/29 07:40:06 mrg Exp $ */

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
 * color display (cgsix) driver; sun4 obio bus front-end.
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
#include <machine/fbvar.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/conf.h>

#include <sparc/dev/btreg.h>
#include <sparc/dev/btvar.h>
#include <sparc/dev/sbusvar.h>
#include <sparc/dev/cgsixreg.h>
#include <sparc/dev/cgsixvar.h>
#include <sparc/dev/pfourreg.h>

/* autoconfiguration driver */
static int	cgsixmatch __P((struct device *, struct cfdata *, void *));
static void	cgsixattach __P((struct device *, struct device *, void *));
static int	cg6_pfour_probe __P((void *, void *));

struct cfattach cgsix_obio_ca = {
	sizeof(struct cgsix_softc), cgsixmatch, cgsixattach
};

/*
 * Match a cgsix.
 */
static int
cgsixmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, 0,
				oba->oba_paddr + CGSIX_FHC_OFFSET,
				4,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				cg6_pfour_probe, NULL));
}

static int
cg6_pfour_probe(vaddr, arg)
	void *vaddr;
	void *arg;
{

	return (fb_pfour_id(vaddr) == PFOUR_ID_FASTCOLOR);
}


/*
 * Attach a display.
 */
static void
cgsixattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cgsix_softc *sc = (struct cgsix_softc *)self;
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;
	struct eeprom *eep = (struct eeprom *)eeprom_va;
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int constype, isconsole;
	char *name;

	oba = &uoba->uoba_oba4;

	/* Remember cookies for cgsix_mmap() */
	sc->sc_bustag = oba->oba_bustag;
	sc->sc_btype = (bus_type_t)0;
	sc->sc_paddr = (bus_addr_t)oba->oba_paddr;

	fb->fb_device = &sc->sc_dev;
	fb->fb_type.fb_type = FBTYPE_SUNFAST_COLOR;
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags & FB_USERMASK;
	fb->fb_type.fb_depth = 8;

	fb_setsize_eeprom(fb, fb->fb_type.fb_depth, 1152, 900);

	/*
	 * Dunno what the PROM has mapped, though obviously it must have
	 * the video RAM mapped.  Just map what we care about for ourselves
	 * (the FHC, THC, and Brooktree registers).
	 */
	if (bus_space_map2(oba->oba_bustag, 0,
			   oba->oba_paddr + CGSIX_BT_OFFSET,
			   sizeof(*sc->sc_bt),
			   BUS_SPACE_MAP_LINEAR,
			   0, &bh) != 0) {
		printf("%s: cannot map brooktree registers\n", self->dv_xname);
		return;
	}
	sc->sc_bt = (struct bt_regs *)bh;

	if (bus_space_map2(oba->oba_bustag, 0,
			   oba->oba_paddr + CGSIX_FHC_OFFSET,
			   sizeof(*sc->sc_fhc),
			   BUS_SPACE_MAP_LINEAR,
			   0, &bh) != 0) {
		printf("%s: cannot map FHC registers\n", self->dv_xname);
		return;
	}
	sc->sc_fhc = (int *)bh;

	if (bus_space_map2(oba->oba_bustag, 0,
			   oba->oba_paddr + CGSIX_THC_OFFSET,
			   sizeof(*sc->sc_thc),
			   BUS_SPACE_MAP_LINEAR,
			   0, &bh) != 0) {
		printf("%s: cannot map THC registers\n", self->dv_xname);
		return;
	}
	sc->sc_thc = (struct cg6_thc *)bh;

	if (bus_space_map2(oba->oba_bustag, 0,
			   oba->oba_paddr + CGSIX_TEC_OFFSET,
			   sizeof(*sc->sc_tec),
			   BUS_SPACE_MAP_LINEAR,
			   0, &bh) != 0) {
		printf("%s: cannot map TEC registers\n", self->dv_xname);
		return;
	}
	sc->sc_tec = (struct cg6_tec_xxx *)bh;

	if (bus_space_map2(oba->oba_bustag, 0,
			   oba->oba_paddr + CGSIX_FBC_OFFSET,
			   sizeof(*sc->sc_fbc),
			   BUS_SPACE_MAP_LINEAR,
			   0, &bh) != 0) {
		printf("%s: cannot map FBC registers\n", self->dv_xname);
		return;
	}
	sc->sc_fbc = (struct cg6_fbc *)bh;


	if (fb_pfour_id((void *)sc->sc_fhc) == PFOUR_ID_FASTCOLOR) {
		fb->fb_flags |= FB_PFOUR;
		name = "cgsix/p4";
	} else
		name = "cgsix";

	constype = (fb->fb_flags & FB_PFOUR) ? EE_CONS_P4OPT : EE_CONS_COLOR;

	/*
	 * Assume this is the console if there's no eeprom info
	 * to be found.
	 */
	if (eep == NULL || eep->eeConsole == constype)
		isconsole = fb_is_console(0);
	else
		isconsole = 0;

	if (isconsole && cgsix_use_rasterconsole) {
		int ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
		if (bus_space_map2(oba->oba_bustag, 0,
				   oba->oba_paddr + CGSIX_RAM_OFFSET,
				   ramsize,
				   BUS_SPACE_MAP_LINEAR,
				   0, &bh) != 0) {
			printf("%s: cannot map pixels\n", self->dv_xname);
			return;
		}
		sc->sc_fb.fb_pixels = (caddr_t)bh;
	}

	cg6attach(sc, name, isconsole);
}
