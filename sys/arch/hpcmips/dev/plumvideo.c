/*	$NetBSD: plumvideo.c,v 1.1 1999/11/21 06:50:27 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "opt_tx39_debug.h"
#include "biconsdev.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumicuvar.h>
#include <hpcmips/dev/plumpowervar.h>
#include <hpcmips/dev/plumvideoreg.h>

#if NBICONSDEV > 0
#include <machine/bootinfo.h>
#include <hpcmips/dev/biconsvar.h>
#include <hpcmips/dev/bicons.h>
#endif /* NBICONSDEV */

int	plumvideo_match __P((struct device*, struct cfdata*, void*));
void	plumvideo_attach __P((struct device*, struct device*, void*));
int	plumvideo_print __P((void*, const char*));

struct plumvideo_softc {
	struct	device		sc_dev;
	plum_chipset_tag_t	sc_pc;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

struct cfattach plumvideo_ca = {
	sizeof(struct plumvideo_softc), plumvideo_match, plumvideo_attach
};

struct fb_attach_args {
	const char *fba_name;
};

void	plumvideo_dump __P((struct plumvideo_softc*));
#if NBICONSDEV > 0
void	plumvideo_biconsinit __P((struct plumvideo_softc*));
#endif /* NBICONSDEV */

int
plumvideo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
plumvideo_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct plum_attach_args *pa = aux;
	struct plumvideo_softc *sc = (void*)self;
	struct fb_attach_args fba;

	sc->sc_pc	= pa->pa_pc;
	sc->sc_regt	= pa->pa_regt;
	sc->sc_iot	= pa->pa_iot;

	if (bus_space_map(sc->sc_regt, PLUM_VIDEO_REGBASE, 
			  PLUM_VIDEO_REGSIZE, 0, &sc->sc_regh)) {
		printf(": register map failed\n");
		return;
	}
	if (bus_space_map(sc->sc_iot, PLUM_VIDEO_VRAM_IOBASE,
			  PLUM_VIDEO_VRAM_IOSIZE, 0, &sc->sc_ioh)) {
		printf(": V-RAM map failed\n");
		return;
	}

	printf("\n");
	plum_power_disestablish(sc->sc_pc, PLUM_PWR_LCD);
	plum_power_disestablish(sc->sc_pc, PLUM_PWR_BKL);
	delay(100);
	plum_power_establish(sc->sc_pc, PLUM_PWR_LCD);
	plum_power_establish(sc->sc_pc, PLUM_PWR_BKL);
	delay(100);

//	plumvideo_dump(sc);
#if NBICONSDEV > 0
	/* Enable builtin console */
	plumvideo_biconsinit(sc);
#endif /* NBICONSDEV */

	/* Attach frame buffer device */
	fba.fba_name = "fb";
	config_found(self, &fba, plumvideo_print);
}

int
plumvideo_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	return pnp ? QUIET : UNCONF;
}


#if NBICONSDEV > 0
void
plumvideo_biconsinit(sc)
	struct plumvideo_softc *sc;
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	bus_space_handle_t ioh = sc->sc_ioh;
	plumreg_t reg;

	reg = plum_conf_read(regt, regh, PLUM_VIDEO_PLGMD_REG);
	switch (reg & PLUM_VIDEO_PLGMD_MASK) {
	default:
		reg |= PLUM_VIDEO_PLGMD_8BPP; /* XXX */
		plum_conf_write(regt, regh, PLUM_VIDEO_PLGMD_REG, reg);
		/* FALLTHROUGH */
	case PLUM_VIDEO_PLGMD_8BPP:
		bootinfo->fb_line_bytes = bootinfo->fb_width * 8 / 8;
		break;
	case PLUM_VIDEO_PLGMD_16BPP:
		bootinfo->fb_line_bytes = bootinfo->fb_width * 16 / 8;
		break;
	}
	
	bootinfo->fb_addr = (unsigned char *)ioh;

	/* re-install */
	bicons_init();
}
#endif /* NBICONSDEV */

void
plumvideo_dump(sc)
	struct plumvideo_softc *sc;
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	plumreg_t reg;
	int i, j;

	for (i = 0; i < 0x200; i+=4) {
		reg = plum_conf_read(regt, regh, i);
		printf("%03x %08x", i, reg);
		bitdisp(reg);
	}
	for (j = 0; j < 10; j++) {
		reg = plum_conf_read(regt, regh, PLUM_VIDEO_PLILN_REG);
		printf("%d, %d\n", reg,
		       plum_conf_read(regt, regh, PLUM_VIDEO_PLCLN_REG));
		for (i = 0; i < PLUM_VIDEO_VRAM_IOSIZE; i += 4) {
			bus_space_write_4(iot, ioh, i, 0);
		}
		for (i = 0; i < PLUM_VIDEO_VRAM_IOSIZE; i += 4) {
			bus_space_write_4(iot, ioh, i, ~0);
		}
	}
}

