/*	$NetBSD: cgsix_obio.c,v 1.25.12.2 2017/12/03 11:36:43 jdolecek Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cgsix_obio.c,v 1.25.12.2 2017/12/03 11:36:43 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#ifdef DEBUG
#include <sys/proc.h>
#include <sys/syslog.h>
#endif

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/eeprom.h>

#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>
#include <dev/sun/btreg.h>
#include <dev/sun/btvar.h>
#include <dev/sun/cgsixreg.h>
#include <dev/sun/cgsixvar.h>
#include <dev/sun/pfourreg.h>

/* autoconfiguration driver */
static int	cgsixmatch(device_t, struct cfdata *, void *);
static void	cgsixattach(device_t, device_t, void *);
static int	cg6_pfour_probe(void *, void *);

CFATTACH_DECL_NEW(cgsix_obio, sizeof(struct cgsix_softc),
    cgsixmatch, cgsixattach, NULL, NULL);

/*
 * Match a cgsix.
 */
static int
cgsixmatch(device_t parent, struct cfdata *cf, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag,
				oba->oba_paddr + CGSIX_FHC_OFFSET,
				4,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				cg6_pfour_probe, NULL));
}

static int
cg6_pfour_probe(void *vaddr, void *arg)
{

	return (fb_pfour_id(vaddr) == PFOUR_ID_FASTCOLOR);
}


/*
 * Attach a display.
 */
static void
cgsixattach(device_t parent, device_t self, void *aux)
{
	struct cgsix_softc *sc = device_private(self);
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;
	struct eeprom *eep = (struct eeprom *)eeprom_va;
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int constype, isconsole;
	const char *name;

	sc->sc_dev = self;
	oba = &uoba->uoba_oba4;

	/* Remember cookies for cgsix_mmap() */
	sc->sc_bustag = oba->oba_bustag;
	sc->sc_paddr = (bus_addr_t)oba->oba_paddr;

	fb->fb_device = sc->sc_dev;
	fb->fb_type.fb_type = FBTYPE_SUNFAST_COLOR;
	fb->fb_flags = device_cfdata(sc->sc_dev)->cf_flags & FB_USERMASK;
	fb->fb_type.fb_depth = 8;

	fb_setsize_eeprom(fb, fb->fb_type.fb_depth, 1152, 900);

	sc->sc_ramsize = 1024 * 1024;	/* All our cgsix's are 1MB */

	/*
	 * Dunno what the PROM has mapped, though obviously it must have
	 * the video RAM mapped.  Just map what we care about for ourselves
	 * (the FHC, THC, and Brooktree registers).
	 */
	if (bus_space_map(oba->oba_bustag,
			  oba->oba_paddr + CGSIX_BT_OFFSET,
			  sizeof(*sc->sc_bt),
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map brooktree registers\n", device_xname(self));
		return;
	}
	sc->sc_bt = (struct bt_regs *)bh;

	if (bus_space_map(oba->oba_bustag,
			  oba->oba_paddr + CGSIX_FHC_OFFSET,
			  sizeof(*sc->sc_fhc),
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map FHC registers\n", device_xname(self));
		return;
	}
	sc->sc_fhc = (int *)bh;

	if (bus_space_map(oba->oba_bustag,
			  oba->oba_paddr + CGSIX_THC_OFFSET,
			  sizeof(*sc->sc_thc),
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map THC registers\n", device_xname(self));
		return;
	}
	sc->sc_thc = (struct cg6_thc *)bh;

	if (bus_space_map(oba->oba_bustag,
			  oba->oba_paddr + CGSIX_TEC_OFFSET,
			  sizeof(*sc->sc_tec),
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map TEC registers\n", device_xname(self));
		return;
	}
	sc->sc_tec = (struct cg6_tec_xxx *)bh;

	if (bus_space_map(oba->oba_bustag,
			  oba->oba_paddr + CGSIX_FBC_OFFSET,
			  sizeof(*sc->sc_fbc),
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map FBC registers\n", device_xname(self));
		return;
	}
	sc->sc_fbc = (struct cg6_fbc *)bh;


	if (fb_pfour_id(sc->sc_fhc) == PFOUR_ID_FASTCOLOR) {
		fb->fb_flags |= FB_PFOUR;
		name = "cgsix/p4";
	} else
		name = "cgsix";

	constype = (fb->fb_flags & FB_PFOUR) ? EE_CONS_P4OPT : EE_CONS_COLOR;

	/*
	 * Check to see if this is the console if there's no eeprom info
	 * to be found, or if it's the correct framebuffer type.
	 */
	if (eep == NULL || eep->eeConsole == constype)
		isconsole = fb_is_console(0);
	else
		isconsole = 0;

	if (bus_space_map(oba->oba_bustag,
			  oba->oba_paddr + CGSIX_RAM_OFFSET,
			  sc->sc_ramsize,
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map pixels\n", device_xname(self));
		return;
	}
	sc->sc_fb.fb_pixels = (void *)bh;

	cg6attach(sc, name, isconsole);
}
