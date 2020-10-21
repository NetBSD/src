/* $NetBSD: genfb_grfbus.c,v 1.2 2020/10/21 11:15:18 rin Exp $ */

/* NetBSD: simplefb.c,v 1.7 2019/01/30 00:55:04 jmcneill Exp */
/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* NetBSD: macfb.c,v 1.20 2012/10/27 17:17:59 chs Exp */
/*
 * Copyright (c) 1998 Matt DeBergalis
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
 *      This product includes software developed by Matt DeBergalis
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

#include "opt_wsdisplay_compat.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: genfb_grfbus.c,v 1.2 2020/10/21 11:15:18 rin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <machine/video.h>
#include <machine/grfioctl.h>
#include <mac68k/nubus/nubus.h>
#include <mac68k/dev/grfvar.h>

#include <dev/wsfb/genfbvar.h>

struct genfb_grfbus_softc {
	struct genfb_softc sc_gen;

	uint32_t sc_width;
	uint32_t sc_height;
	uint8_t  sc_depth;
	uint16_t sc_stride;

	paddr_t sc_paddr;
	vaddr_t sc_vaddr;

	void (*sc_set_mapreg)(void *, int, int, int, int);
	struct grfbus_softc *sc_parent;
};

static int genfb_grfbus_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t genfb_grfbus_mmap(void *, void *, off_t, int);

static struct genfb_ops gfb_ops = {
	.genfb_ioctl = genfb_grfbus_ioctl,
	.genfb_mmap = genfb_grfbus_mmap,
};

static struct genfb_colormap_callback gfb_cmcb;

static int __unused
genfb_grfbus_is_console(paddr_t addr)
{

	if (addr != mac68k_video.mv_phys &&
	    (addr >= 0xf9000000 && addr <= 0xfeffffff)) {
		/*
		 * This is in the NuBus standard slot space range, so we
		 * may well have to look at 0xFssxxxxx, too.  Mask off the
		 * slot number and duplicate it in bits 20-23, per IM:V
		 * pp 459, 463, and IM:VI ch 30 p 17.
		 * Note:  this is an ugly hack and I wish I knew what
		 * to do about it.  -- sr
		 */
		addr = (paddr_t)(((u_long)addr & 0xff0fffff) |
		    (((u_long)addr & 0x0f000000) >> 4));
	}
	return ((mac68k_machine.serial_console & 0x03) == 0
	    && (addr == mac68k_video.mv_phys));
}

static int
genfb_grfbus_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct genfb_grfbus_softc * const sc = v;
#if 0
	struct wsdisplayio_bus_id *busid;
#endif
	struct wsdisplayio_fbinfo *fbi;
	struct rasops_info *ri;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_GENFB;
		return 0;
#if 0
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_GRFBUS;
		return 0;
#endif
	case WSDISPLAYIO_GET_FBINFO:
		fbi = data;
		ri = &sc->sc_gen.vd.active->scr_ri;
		error = wsdisplayio_get_fbinfo(ri, fbi);
		if (error == 0) {
	                /*
	                 * XXX
	                 * if the fb isn't page aligned, tell wsfb to skip the
			 * unaligned part
			 */
			fbi->fbi_fboffset = m68k_page_offset(sc->sc_paddr);
		}
		return error;
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
genfb_grfbus_mmap(void *v, void *vs, off_t off, int prot)
{
	struct genfb_grfbus_softc * const sc = v;

	if (off < 0 || off >= m68k_page_offset(sc->sc_paddr) +
	    sc->sc_gen.sc_fbsize)
		return -1;

	return m68k_btop(m68k_trunc_page(sc->sc_paddr) + off);
}

static int
genfb_grfbus_attach_genfb(struct genfb_grfbus_softc *sc)
{
	device_t dev = sc->sc_gen.sc_dev;
	prop_dictionary_t dict = device_properties(dev);
	bool is_console;

	prop_dictionary_set_uint32(dict, "width", sc->sc_width);
	prop_dictionary_set_uint32(dict, "height", sc->sc_height);
	prop_dictionary_set_uint8(dict, "depth", sc->sc_depth);
	prop_dictionary_set_uint16(dict, "linebytes", sc->sc_stride);

	prop_dictionary_set_uint32(dict, "address", sc->sc_paddr);
	prop_dictionary_set_uint64(dict, "virtual_address", sc->sc_vaddr);

	if (sc->sc_set_mapreg != NULL) {
		gfb_cmcb.gcc_cookie = sc->sc_parent;
		gfb_cmcb.gcc_set_mapreg = sc->sc_set_mapreg;
		prop_dictionary_set_uint64(dict, "cmap_callback",
		    (uint64_t)(uintptr_t)&gfb_cmcb);
	}

	genfb_init(&sc->sc_gen);

	if (sc->sc_gen.sc_width == 0 || sc->sc_gen.sc_fbsize == 0) {
		aprint_normal(": disabled\n");
		return ENXIO;
	}

	aprint_naive("\n");
	aprint_normal(": colormap callback %sprovided\n",
	    sc->sc_set_mapreg != NULL ? "" : "not ");

	is_console = genfb_grfbus_is_console(sc->sc_paddr);
	if (is_console)
		aprint_normal_dev(sc->sc_gen.sc_dev,
		    "switching to framebuffer console\n");
	prop_dictionary_set_bool(dict, "is_console", is_console);

	genfb_attach(&sc->sc_gen, &gfb_ops);

	return 0;
}

static int
genfb_grfbus_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
genfb_grfbus_attach(device_t parent, device_t self, void *aux)
{
	struct genfb_grfbus_softc * const sc = device_private(self);
	struct grfbus_attach_args *ga = aux;
	struct grfmode *gm = ga->ga_grfmode;

	sc->sc_gen.sc_dev = self;

	sc->sc_width = gm->width;
	sc->sc_height = gm->height;

	/*
	 * XXX: tested on Quadra 840AV
	 */
	sc->sc_depth = gm->psize == 16 ? 15 : gm->psize;

	sc->sc_stride = gm->rowbytes;
	sc->sc_paddr = ga->ga_phys + gm->fboff;
	sc->sc_vaddr = (vaddr_t)gm->fbbase + gm->fboff;
	sc->sc_set_mapreg = ga->ga_set_mapreg;
	sc->sc_parent = ga->ga_parent;

	genfb_grfbus_attach_genfb(sc);
}

CFATTACH_DECL_NEW(genfb_grfbus, sizeof(struct genfb_grfbus_softc),
	genfb_grfbus_match, genfb_grfbus_attach, NULL, NULL);
