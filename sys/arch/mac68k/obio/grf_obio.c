/*	$NetBSD: grf_obio.c,v 1.57.54.1 2012/10/30 17:19:56 yamt Exp $	*/

/*
 * Copyright (C) 1998 Scott Reynolds
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/*
 * Copyright (c) 1995 Allen Briggs.  All rights reserved.
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
 *	This product includes software developed by Allen Briggs.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/*
 * Graphics display driver for the Macintosh internal video for machines
 * that don't map it into a fake nubus card.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf_obio.c,v 1.57.54.1 2012/10/30 17:19:56 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/grfioctl.h>
#include <machine/viareg.h>
#include <machine/video.h>

#include <mac68k/nubus/nubus.h>
#include <mac68k/obio/obiovar.h>
#include <mac68k/dev/grfvar.h>

static int	grfiv_mode(struct grf_softc *, int, void *);
static int	grfiv_match(device_t, cfdata_t, void *);
static void	grfiv_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(intvid, sizeof(struct grfbus_softc),
    grfiv_match, grfiv_attach, NULL, NULL);

#define	DAFB_BASE		0xf9000000
#define DAFB_CONTROL_BASE	0xf9800000
#define	CIVIC_BASE		0x50100000
#define CIVIC_CONTROL_BASE	0x50036000
#define	VALKYRIE_BASE		0xf9000000
#define VALKYRIE_CONTROL_BASE	0x50f2a000

static int
grfiv_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	bus_space_handle_t bsh;
	int found;
	u_int base;

	found = 1;

	switch (current_mac_model->class) {
	case MACH_CLASSQ2:
		if (current_mac_model->machineid != MACH_MACLC575) {
			base = VALKYRIE_CONTROL_BASE;

			if (bus_space_map(oa->oa_tag, base, 0x40, 0, &bsh))
				return 0;
			
			/* Disable interrupts */
			bus_space_write_1(oa->oa_tag, bsh, 0x18, 0x1);

			bus_space_unmap(oa->oa_tag, bsh, 0x40);
			break;
		}
		/*
		 * Note:  the only system in this class that does not have
		 * the Valkyrie chip -- at least, that we know of -- is
		 * the Performa/LC 57x series.  This system has a version
		 * of the DAFB controller, instead.
		 *
		 * If this assumption proves false, we'll have to be more
		 * intelligent here.
		 */
		/*FALLTHROUGH*/
	case MACH_CLASSQ:
		/*
		 * Assume DAFB for all of these, unless we can't
		 * access the memory.
		 */
		base = DAFB_CONTROL_BASE;

		if (bus_space_map(oa->oa_tag, base, 0x20, 0, &bsh))
			return 0;

		if (mac68k_bus_space_probe(oa->oa_tag, bsh, 0x1c, 4) == 0) {
			bus_space_unmap(oa->oa_tag, bsh, 0x20);
			return 0;
		}

		bus_space_unmap(oa->oa_tag, bsh, 0x20);

		if (bus_space_map(oa->oa_tag, base + 0x100, 0x20, 0, &bsh))
			return 0;

		if (mac68k_bus_space_probe(oa->oa_tag, bsh, 0x04, 4) == 0) {
			bus_space_unmap(oa->oa_tag, bsh, 0x20);
			return 0;
		}

		/* Disable interrupts */
		bus_space_write_4(oa->oa_tag, bsh, 0x04, 0);

		/* Clear any interrupts */
		bus_space_write_4(oa->oa_tag, bsh, 0x0C, 0);
		bus_space_write_4(oa->oa_tag, bsh, 0x10, 0);
		bus_space_write_4(oa->oa_tag, bsh, 0x14, 0);

		bus_space_unmap(oa->oa_tag, bsh, 0x20);
		break;
	case MACH_CLASSAV:
		base = CIVIC_CONTROL_BASE;

		if (bus_space_map(oa->oa_tag, base, 0x1000, 0, &bsh))
			return 0;

		/* Disable interrupts */
		bus_space_write_1(oa->oa_tag, bsh, 0x120, 0);

		bus_space_unmap(oa->oa_tag, bsh, 0x1000);
		break;
	case MACH_CLASSIIci:
	case MACH_CLASSIIsi:
		if (mac68k_video.mv_len == 0 ||
		    (via2_reg(rMonitor) & RBVMonitorMask) == RBVMonIDNone)
			found = 0;
		break;
	default:
		if (mac68k_video.mv_len == 0)
			found = 0;
		break;
	}

	return found;
}

static void
grfiv_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	struct grfbus_softc *sc;
	struct grfmode *gm;
	u_long base, length;
	u_int32_t vbase1, vbase2;

	sc = device_private(self);
	sc->sc_dev = self;

	sc->card_id = 0;

	switch (current_mac_model->class) {
	case MACH_CLASSQ2:
		if (current_mac_model->machineid != MACH_MACLC575) {
			sc->sc_basepa = VALKYRIE_BASE;
			length = 0x00100000;		/* 1MB */

			if (sc->sc_basepa <= mac68k_video.mv_phys &&
			    mac68k_video.mv_phys < (sc->sc_basepa + length)) {
				sc->sc_fbofs =
				    mac68k_video.mv_phys - sc->sc_basepa;
			} else {
				sc->sc_basepa =
				    m68k_trunc_page(mac68k_video.mv_phys);
				sc->sc_fbofs =
				    m68k_page_offset(mac68k_video.mv_phys);
				length = mac68k_video.mv_len + sc->sc_fbofs;
			}

			printf(" @ %lx: Valkyrie video subsystem\n",
			    sc->sc_basepa + sc->sc_fbofs);
			break;
		}
		/* See note in grfiv_match() */
		/*FALLTHROUGH*/
	case MACH_CLASSQ:
		base = DAFB_CONTROL_BASE;
		sc->sc_tag = oa->oa_tag;
		if (bus_space_map(sc->sc_tag, base, 0x20, 0, &sc->sc_regh)) {
			printf(": failed to map DAFB register space\n");
			return;
		}

		sc->sc_basepa = DAFB_BASE;
		length = 0x00100000;		/* 1MB */

		/* Compute the current frame buffer offset */
		vbase1 = bus_space_read_4(sc->sc_tag, sc->sc_regh, 0x0) & 0xfff;
#if 1
		/*
		 * XXX The following exists because the DAFB v7 in these
		 * systems doesn't return reasonable values to use for fbofs.
		 * Ken'ichi Ishizaka gets credit for this hack.  (sar 19990426)
		 * (Does this get us the correct result for _all_ DAFB-
		 * equipped systems and monitor combinations?  It seems
		 * possible, if not likely...)
		 */
		switch (current_mac_model->machineid) {
		case MACH_MACLC475:
		case MACH_MACLC475_33:
		case MACH_MACLC575:
		case MACH_MACQ605:
		case MACH_MACQ605_33:
			vbase1 &= 0x3f;
			break;
		}
#endif
		vbase2 = bus_space_read_4(sc->sc_tag, sc->sc_regh, 0x4) & 0xf;
		sc->sc_fbofs = (vbase1 << 9) | (vbase2 << 5);

		printf(" @ %lx: DAFB video subsystem, monitor sense %x\n",
		    sc->sc_basepa + sc->sc_fbofs,
		    (bus_space_read_4(sc->sc_tag, sc->sc_regh, 0x1c) & 0x7));

		bus_space_unmap(sc->sc_tag, sc->sc_regh, 0x20);
		break;
	case MACH_CLASSAV:
		sc->sc_basepa = CIVIC_BASE;
		length = 0x00200000;		/* 2MB */
		if (mac68k_video.mv_phys >= sc->sc_basepa &&
		    mac68k_video.mv_phys < (sc->sc_basepa + length)) {
			sc->sc_fbofs = mac68k_video.mv_phys - sc->sc_basepa;
		} else {
			sc->sc_basepa = m68k_trunc_page(mac68k_video.mv_phys);
			sc->sc_fbofs = m68k_page_offset(mac68k_video.mv_phys);
			length = mac68k_video.mv_len + sc->sc_fbofs;
		}

		printf(" @ %lx: CIVIC video subsystem\n",
		    sc->sc_basepa + sc->sc_fbofs);
		break;
	case MACH_CLASSIIci:
	case MACH_CLASSIIsi:
		sc->sc_basepa = m68k_trunc_page(mac68k_video.mv_phys);
		sc->sc_fbofs = m68k_page_offset(mac68k_video.mv_phys);
		length = mac68k_video.mv_len + sc->sc_fbofs;

		printf(" @ %lx: RBV video subsystem, ",
		    sc->sc_basepa + sc->sc_fbofs);
		switch (via2_reg(rMonitor) & RBVMonitorMask) {
		case RBVMonIDBWP:
			printf("15\" monochrome portrait");
			break;
		case RBVMonIDRGB12:
			printf("12\" color");
			break;
		case RBVMonIDRGB15:
			printf("15\" color");
			break;
		case RBVMonIDStd:
			printf("Macintosh II");
			break;
		default:
			printf("unrecognized");
			break;
		}
		printf(" display\n");

		break;
	default:
		sc->sc_basepa = m68k_trunc_page(mac68k_video.mv_phys);
		sc->sc_fbofs = m68k_page_offset(mac68k_video.mv_phys);
		length = mac68k_video.mv_len + sc->sc_fbofs;

		printf(" @ %lx: On-board video\n",
		    sc->sc_basepa + sc->sc_fbofs);
		break;
	}

	if (bus_space_map(sc->sc_tag, sc->sc_basepa, length, 0,
	    &sc->sc_handle)) {
		printf("%s: failed to map video RAM\n", device_xname(sc->sc_dev));
		return;
	}

	if (sc->sc_basepa <= mac68k_video.mv_phys &&
	    mac68k_video.mv_phys < (sc->sc_basepa + length)) {
		/* XXX Hack */
		mac68k_video.mv_kvaddr = sc->sc_handle.base + sc->sc_fbofs;
	}

	gm = &(sc->curr_mode);
	gm->mode_id = 0;
	gm->psize = mac68k_video.mv_depth;
	gm->ptype = 0;
	gm->width = mac68k_video.mv_width;
	gm->height = mac68k_video.mv_height;
	gm->rowbytes = mac68k_video.mv_stride;
	gm->hres = 80;				/* XXX hack */
	gm->vres = 80;				/* XXX hack */
	gm->fbsize = gm->height * gm->rowbytes;
	gm->fbbase = (void *)sc->sc_handle.base; /* XXX yet another hack */
	gm->fboff = sc->sc_fbofs;

	/* Perform common video attachment. */
	grf_establish(sc, NULL, grfiv_mode);
}

static int
grfiv_mode(struct grf_softc *sc, int cmd, void *arg)
{
	switch (cmd) {
	case GM_GRFON:
	case GM_GRFOFF:
		return 0;
	case GM_CURRMODE:
		break;
	case GM_NEWMODE:
		break;
	case GM_LISTMODES:
		break;
	}
	return EINVAL;
}
