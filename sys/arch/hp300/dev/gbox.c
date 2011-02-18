/*	$NetBSD: gbox.c,v 1.3 2011/02/18 19:15:43 tsutsui Exp $	*/
/*	$OpenBSD: gbox.c,v 1.15 2007/01/07 15:13:52 miod Exp $	*/

/*
 * Copyright (c) 2005, Miodrag Vallat
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Copyright (c) 1988 University of Utah.
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
 * from: Utah $Hdr: grf_gb.c 1.18 93/08/13$
 *
 *	@(#)grf_gb.c	8.4 (Berkeley) 1/12/94
 */

/*
 * Graphics routines for the Gatorbox.
 *
 * Note: In the context of this system, "gator" and "gatorbox" both refer to
 *       HP 987x0 graphics systems.  "Gator" is not used for high res mono.
 *       (as in 9837 Gator systems)
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <machine/autoconf.h>

#include <hp300/dev/dioreg.h>
#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>
#include <hp300/dev/intiovar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <hp300/dev/diofbreg.h>
#include <hp300/dev/diofbvar.h>
#include <hp300/dev/gboxreg.h>

struct	gbox_softc {
	device_t	sc_dev;
	struct diofb	*sc_fb;
	struct diofb	sc_fb_store;
	int		sc_scode;
};

static int	gbox_dio_match(device_t, cfdata_t, void *);
static void	gbox_dio_attach(device_t, device_t, void *);
static int	gbox_intio_match(device_t, cfdata_t, void *);
static void	gbox_intio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gbox_dio, sizeof(struct gbox_softc),
    gbox_dio_match, gbox_dio_attach, NULL, NULL);

CFATTACH_DECL_NEW(gbox_intio, sizeof(struct gbox_softc),
    gbox_intio_match, gbox_intio_attach, NULL, NULL);

static int	gbox_reset(struct diofb *, int, struct diofbreg *);
static void	gbox_restore(struct diofb *);
static int	gbox_setcmap(struct diofb *, struct wsdisplay_cmap *);
static void	gbox_setcolor(struct diofb *, u_int);
static int	gbox_windowmove(struct diofb *, uint16_t, uint16_t, uint16_t,
		    uint16_t, uint16_t, uint16_t, int16_t, int16_t);

static int	gbox_ioctl(void *, void *, u_long, void *, int, struct lwp *);

static struct wsdisplay_accessops gbox_accessops = {
	gbox_ioctl,
	diofb_mmap,
	diofb_alloc_screen,
	diofb_free_screen,
	diofb_show_screen,
	NULL,   /* load_font */
};

/*
 * Attachment glue
 */
int
gbox_intio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	struct diofbreg *fbr;

	if (strcmp("fb", ia->ia_modname) != 0)
		return 0;

	fbr = (struct diofbreg *)ia->ia_addr;

	if (badaddr((void *)fbr))
		return 0;

	if (fbr->id == GRFHWID && fbr->fbid == GID_GATORBOX) {
		return 1;
	}

	return 0;
}

void
gbox_intio_attach(device_t parent, device_t self, void *aux)
{
	struct gbox_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	struct diofbreg *fbr;

	sc->sc_dev = self;
	fbr = (struct diofbreg *)ia->ia_addr;
	sc->sc_scode = CONSCODE_INTERNAL;

	if (sc->sc_scode == conscode) {
		sc->sc_fb = &diofb_cn;
	} else {
		sc->sc_fb = &sc->sc_fb_store;
		gbox_reset(sc->sc_fb, sc->sc_scode, fbr);
	}

	diofb_end_attach(self, &gbox_accessops, sc->sc_fb,
	    sc->sc_scode == conscode, NULL);
}

int
gbox_dio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct dio_attach_args *da = aux;

	/* We can not appear in DIO-II space */
	if (DIO_ISDIOII(da->da_scode))
		return 0;

	if (da->da_id == DIO_DEVICE_ID_FRAMEBUFFER &&
	    da->da_secid == DIO_DEVICE_SECID_GATORBOX)
		return 1;

	return 0;
}

void
gbox_dio_attach(device_t parent, device_t self, void *aux)
{
	struct gbox_softc *sc = device_private(self);
	struct dio_attach_args *da = aux;
	bus_space_handle_t bsh;
	struct diofbreg * fbr;

	sc->sc_dev = self;
	sc->sc_scode = da->da_scode;
	if (sc->sc_scode == conscode) {
		fbr = (struct diofbreg *)conaddr;	/* already mapped */
		sc->sc_fb = &diofb_cn;
	} else {
		sc->sc_fb = &sc->sc_fb_store;
		if (bus_space_map(da->da_bst, da->da_addr, da->da_size, 0,
		    &bsh)) {
			aprint_error(": can't map framebuffer\n");
			return;
		}
		fbr = bus_space_vaddr(da->da_bst, bsh);
		if (gbox_reset(sc->sc_fb, sc->sc_scode, fbr) != 0) {
			aprint_error(": can't reset framebuffer\n");
			return;
		}
	}

	diofb_end_attach(self, &gbox_accessops, sc->sc_fb,
	    sc->sc_scode == conscode, NULL);
}

/*
 * Initialize hardware and display routines.
 */

const uint8_t crtc_init_data[] = {
    0x29, 0x20, 0x23, 0x04, 0x30, 0x0b, 0x30,
    0x30, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00
};

int
gbox_reset(struct diofb *fb, int scode, struct diofbreg *fbr)
{
	int rc;
	u_int i;

	/* XXX don't trust hardware, force defaults */
	fb->fbwidth = 1024;
	fb->fbheight = 1024;
	fb->dwidth = 1024;
	fb->dheight = 768;
	if ((rc = diofb_fbinquire(fb, scode, fbr)) != 0)
		return rc;

	fb->bmv = gbox_windowmove;
	gbox_restore(fb);

	/*
	 * Find out how many colors are available by determining
	 * which planes are installed.  That is, write all ones to
	 * a frame buffer location, see how many ones are read back.
	 */
	if (1 /* fb->planes == 0 */) {
		volatile uint8_t *fbp;
		uint8_t save;

		fbp = (uint8_t *)fb->fbkva;
		save = *fbp;
		*fbp = 0xff;
		fb->planemask = *fbp;
		*fbp = save;

		for (fb->planes = 1; fb->planemask >= (1 << fb->planes);
		    fb->planes++);
		if (fb->planes > 8)
			fb->planes = 8;
		fb->planemask = (1 << fb->planes) - 1;
	}

	diofb_fbsetup(fb);
	for (i = 0; i <= fb->planemask; i++)
		gbox_setcolor(fb, i);

	return 0;
}

void
gbox_restore(struct diofb *fb)
{
	volatile struct gboxfb *gb = (struct gboxfb *)fb->regkva;
	u_int i;

	/*
	 * The minimal info here is from the Gatorbox X driver.
	 */
	gb->write_protect = 0x0;
	gb->regs.interrupt = 0x4;
	gb->rep_rule = RR_COPY;
	gb->blink1 = 0xff;
	gb->blink2 = 0xff;

	/*
	 * Program the 6845.
	 */
	for (i = 0; i < sizeof(crtc_init_data); i++) {
		gb->crtc_address = i;
		gb->crtc_data = crtc_init_data[i];
	}

	tile_mover_waitbusy(gb);

	/* Enable display */
	gb->regs.sec_interrupt = 0x01;
}

int
gbox_ioctl(void *v, void *vs, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct diofb *fb = v;
	struct wsdisplay_fbinfo *wdf;
	u_int i;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_GBOX;
		return 0;
	case WSDISPLAYIO_SMODE:
		fb->mapmode = *(u_int *)data;
		if (fb->mapmode == WSDISPLAYIO_MODE_EMUL) {
			gbox_restore(fb);
			for (i = 0; i <= fb->planemask; i++)
				gbox_setcolor(fb, i);
		}
		return 0;
	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->width = fb->ri.ri_width;
		wdf->height = fb->ri.ri_height;
		wdf->depth = fb->ri.ri_depth;
		wdf->cmsize = 1 << fb->planes;
		return 0;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = fb->ri.ri_stride;
		return 0;
	case WSDISPLAYIO_GETCMAP:
		return diofb_getcmap(fb, (struct wsdisplay_cmap *)data);
	case WSDISPLAYIO_PUTCMAP:
		return gbox_setcmap(fb, (struct wsdisplay_cmap *)data);
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_SVIDEO:
		return EPASSTHROUGH;
	}

	return EPASSTHROUGH;
}

void
gbox_setcolor(struct diofb *fb, u_int index)
{
	volatile struct gboxfb *gb = (struct gboxfb *)fb->regkva;

	gb->creg_select = index;
	gb->cmap_red = fb->cmap.r[index];
	gb->cmap_grn = fb->cmap.g[index];
	gb->cmap_blu = fb->cmap.b[index];
	gb->cmap_write = !!index;
	gbcm_waitbusy(gb);
}

int
gbox_setcmap(struct diofb *fb, struct wsdisplay_cmap *cm)
{
	uint8_t r[256], g[256], b[256];
	u_int index = cm->index, count = cm->count;
	u_int colcount = 1 << fb->planes;
	int error;

	if (index >= colcount || count > colcount - index)
		return EINVAL;

	if ((error = copyin(cm->red, r, count)) != 0)
		return error;
	if ((error = copyin(cm->green, g, count)) != 0)
		return error;
	if ((error = copyin(cm->blue, b, count)) != 0)
		return error;

	memcpy(fb->cmap.r + index, r, count);
	memcpy(fb->cmap.g + index, g, count);
	memcpy(fb->cmap.b + index, b, count);

	while (count-- != 0)
		gbox_setcolor(fb, index++);

	return 0;
}

int
gbox_windowmove(struct diofb *fb, uint16_t sx, uint16_t sy,
    uint16_t dx, uint16_t dy, uint16_t cx, uint16_t cy, int16_t rop,
    int16_t planemask)
{
	volatile struct gboxfb *gb = (struct gboxfb *)fb->regkva;
	int src, dest;

	if (planemask != 0xff)
		return EINVAL;

	src  = (sy * 1024) + sx; /* upper left corner in pixels */
	dest = (dy * 1024) + dx;

	tile_mover_waitbusy(gb);

	gb->width = -(cx / 4);
	gb->height = -(cy / 4);
	if (src < dest)
		gb->rep_rule = MOVE_DOWN_RIGHT | rop;
	else {
		gb->rep_rule = MOVE_UP_LEFT | rop;
		/*
		 * Adjust to top of lower right tile of the block.
		 */
		src = src + ((cy - 4) * 1024) + (cx - 4);
		dest= dest + ((cy - 4) * 1024) + (cx - 4);
	}
	*(volatile uint8_t *)(fb->fbkva + dest) =
	    *(volatile uint8_t *)(fb->fbkva + src);

	tile_mover_waitbusy(gb);

	return 0;
}

/*
 * Gatorbox console support
 */
int
gboxcnattach(bus_space_tag_t bst, bus_addr_t addr, int scode)
{
	bus_space_handle_t bsh;
	void *va;
	struct diofbreg *fbr;
	struct diofb *fb = &diofb_cn;
	int size;

	if (bus_space_map(bst, addr, PAGE_SIZE, 0, &bsh))
		return 1;
	va = bus_space_vaddr(bst, bsh);
	fbr = va;

	if (badaddr(va) ||
	    (fbr->id != GRFHWID) || (fbr->fbid != GID_GATORBOX)) {
		bus_space_unmap(bst, bsh, PAGE_SIZE);
		return 1;
	}

	size = DIO_SIZE(scode, va);

	bus_space_unmap(bst, bsh, PAGE_SIZE);
	if (bus_space_map(bst, addr, size, 0, &bsh))
		return 1;
	va = bus_space_vaddr(bst, bsh);

	/*
	 * Initialize the framebuffer hardware.
	 */
	conscode = scode;
	conaddr = va;
	gbox_reset(fb, conscode, (struct diofbreg *)conaddr);

	/*
	 * Initialize the terminal emulator.
	 */
	diofb_cnattach(fb);
	return 0;
}
