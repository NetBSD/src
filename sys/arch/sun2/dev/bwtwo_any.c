/*	$NetBSD: bwtwo_any.c,v 1.4 2002/03/22 00:24:45 fredette Exp $ */

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Matthew Fredette.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)bwtwo.c	8.1 (Berkeley) 6/11/93
 */

/*
 * black&white display (bwtwo) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * P4 and overlay plane support by Jason R. Thorpe <thorpej@NetBSD.ORG>.
 * Overlay plane handling hints and ideas provided by Brad Spencer.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <machine/autoconf.h>
#include <machine/conf.h>
#include <sun2/sun2/control.h>

#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>
#include <dev/sun/btreg.h>
#include <sun2/dev/bwtworeg.h>
#include <dev/sun/bwtwovar.h>

/* autoconfiguration driver */
static void	bwtwoattach_any (struct device *, struct device *, void *);
static int	bwtwomatch_any (struct device *, struct cfdata *, void *);

struct bwtwosun2_softc {
	struct bwtwo_softc sc;
	bus_space_handle_t bh;
};

struct cfattach bwtwo_obio_ca = {
	sizeof(struct bwtwosun2_softc), bwtwomatch_any, bwtwoattach_any
};

struct cfattach bwtwo_obmem_ca = {
	sizeof(struct bwtwosun2_softc), bwtwomatch_any, bwtwoattach_any
};

static int	bwtwo_get_video_sun2 __P((struct bwtwo_softc *));
static void	bwtwo_set_video_sun2 __P((struct bwtwo_softc *, int));

extern int fbnode;

static int
bwtwomatch_any(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;
	int matched;

	/* Make sure there is something there... */
	if (bus_space_map(ma->ma_bustag, ma->ma_paddr + BWREG_REG, sizeof(struct bwtworeg), 
			  0, &bh))
		return (0);
	matched = (bus_space_peek_1(ma->ma_bustag, bh, 0, NULL) == 0);
	bus_space_unmap(ma->ma_bustag, bh, sizeof(struct bwtworeg));
	return (matched);
}

static void
bwtwoattach_any(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct bwtwosun2_softc *scsun2 = (struct bwtwosun2_softc *)self;
	struct bwtwo_softc *sc = &scsun2->sc;
	struct mainbus_attach_args *ma = aux;
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int isconsole;
	char *name;

	/* Remember cookies for bwtwo_mmap() */
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_paddr = ma->ma_paddr;

	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags;
	fb->fb_type.fb_depth = 1;
	fb_setsize_eeprom(fb, fb->fb_type.fb_depth, 1152, 900);

	isconsole = fb_is_console(0);

	/* A plain bwtwo */
	sc->sc_reg = NULL;
	fb->fb_pfour = NULL;
	name = "bwtwo";
	sc->sc_pixeloffset = BWREG_MEM;
	sc->sc_get_video = bwtwo_get_video_sun2;
	sc->sc_set_video = bwtwo_set_video_sun2;

	/* Map the registers. */
	if (bus_space_map(ma->ma_bustag, ma->ma_paddr + BWREG_REG, sizeof(struct bwtworeg), 
	    		  0, &scsun2->bh)) {
		printf("%s: cannot map regs\n", self->dv_xname);
		return;
	}

	if (isconsole) {
		int ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
		if (bus_space_map(ma->ma_bustag, ma->ma_paddr + sc->sc_pixeloffset,
				  ramsize,
				  BUS_SPACE_MAP_LINEAR, &bh) != 0) {
			printf("%s: cannot map pixels\n", self->dv_xname);
			return;
		}
		sc->sc_fb.fb_pixels = (char *)bh;
	}

	bwtwoattach(sc, name, isconsole);
}

static void
bwtwo_set_video_sun2(sc, enable)
	struct bwtwo_softc *sc;
	int enable;
{
	struct bwtwosun2_softc *scsun2 = (struct bwtwosun2_softc *)sc;
	unsigned char cr;

	cr = bus_space_read_1(sc->sc_bustag, scsun2->bh, 0);
	bus_space_write_1(sc->sc_bustag, scsun2->bh, 0, (enable ? 
							 (cr | BW2_CR_ENABLE_VIDEO) : 
	    						 (cr & ~BW2_CR_ENABLE_VIDEO)));
	return;
}

static int
bwtwo_get_video_sun2(sc)
	struct bwtwo_softc *sc;
{
	struct bwtwosun2_softc *scsun2 = (struct bwtwosun2_softc *)sc;

	return ((bus_space_read_1(sc->sc_bustag, scsun2->bh, 0) & BW2_CR_ENABLE_VIDEO) != 0);
}
