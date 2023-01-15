/*	$NetBSD: rbox.c,v 1.4 2023/01/15 06:19:45 tsutsui Exp $	*/
/*	$OpenBSD: rbox.c,v 1.14 2006/08/11 18:33:13 miod Exp $	*/

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
 * from: Utah $Hdr: grf_rb.c 1.15 93/08/13$
 *
 *	@(#)grf_rb.c	8.4 (Berkeley) 1/12/94
 */

/*
 * Graphics routines for the Renaissance, HP98720 Graphics system.
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
#include <hp300/dev/rboxreg.h>

struct	rbox_softc {
	device_t	sc_dev;
	struct diofb	*sc_fb;
	struct diofb	sc_fb_store;
	int		sc_scode;
};

static int	rbox_dio_match(device_t, cfdata_t, void *);
static void	rbox_dio_attach(device_t, device_t, void *);
static int	rbox_intio_match(device_t, cfdata_t, void *);
static void	rbox_intio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rbox_dio, sizeof(struct rbox_softc),
    rbox_dio_match, rbox_dio_attach, NULL, NULL);

CFATTACH_DECL_NEW(rbox_intio, sizeof(struct rbox_softc),
    rbox_intio_match, rbox_intio_attach, NULL, NULL);

static int	rbox_reset(struct diofb *, int, struct diofbreg *);
static void	rbox_restore(struct diofb *);
static int	rbox_windowmove(struct diofb *, uint16_t, uint16_t, uint16_t,
		    uint16_t, uint16_t, uint16_t, int16_t, int16_t);

static int	rbox_ioctl(void *, void *, u_long, void *, int, struct lwp *);

static struct wsdisplay_accessops rbox_accessops = {
	rbox_ioctl,
	diofb_mmap,
	diofb_alloc_screen,
	diofb_free_screen,
	diofb_show_screen,
	NULL,	/* load_font */
};

/*
 * Attachment glue
 */

int
rbox_intio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	struct diofbreg *fbr;

	if (strcmp("fb", ia->ia_modname) != 0)
		return 0;

	fbr = (struct diofbreg *)ia->ia_addr;

	if (badaddr((void *)fbr))
		return 0;

	if (fbr->id == GRFHWID && fbr->fbid == GID_RENAISSANCE) {
		return 1;
	}

	return 0;
}

void
rbox_intio_attach(device_t parent, device_t self, void *aux)
{
	struct rbox_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	struct diofbreg *fbr;

	sc->sc_dev = self;
	fbr = (struct diofbreg *)ia->ia_addr;
	sc->sc_scode = CONSCODE_INTERNAL;

	if (sc->sc_scode == conscode) {
		sc->sc_fb = &diofb_cn;
	} else {
		sc->sc_fb = &sc->sc_fb_store;
		rbox_reset(sc->sc_fb, sc->sc_scode, fbr);
	}

	diofb_end_attach(self, &rbox_accessops, sc->sc_fb,
	    sc->sc_scode == conscode, NULL);
}

int
rbox_dio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct dio_attach_args *da = aux;

	if (da->da_id == DIO_DEVICE_ID_FRAMEBUFFER &&
	    da->da_secid == DIO_DEVICE_SECID_RENAISSANCE)
		return 1;

	return 0;
}

void
rbox_dio_attach(device_t parent, device_t self, void *aux)
{
	struct rbox_softc *sc = device_private(self);
	struct dio_attach_args *da = aux;
	bus_space_handle_t bsh;
	struct diofbreg *fbr;

	sc->sc_dev = self;
	sc->sc_scode = da->da_scode;
	if (sc->sc_scode == conscode) {
		fbr = (struct diofbreg *)conaddr;	/* already mapped */
		sc->sc_fb = &diofb_cn;
	} else {
		sc->sc_fb = &sc->sc_fb_store;
		if (bus_space_map(da->da_bst, da->da_addr, da->da_size,
		    0, &bsh)) {
			aprint_error(": can't map framebuffer\n");
			return;
		}
		fbr = bus_space_vaddr(da->da_bst, bsh);
		if (rbox_reset(sc->sc_fb, sc->sc_scode, fbr) != 0) {
			aprint_error(": can't reset framebuffer\n");
			return;
		}
	}

	diofb_end_attach(self, &rbox_accessops, sc->sc_fb,
	    sc->sc_scode == conscode, NULL);
}

/*
 * Initialize hardware and display routines.
 */
int
rbox_reset(struct diofb *fb, int scode, struct diofbreg *fbr)
{
	int rc;

	if ((rc = diofb_fbinquire(fb, scode, fbr)) != 0)
		return rc;

	/*
	 * Restrict the framebuffer to a monochrome view for now, until
	 * I know better how to detect and frob overlay planes, and
	 * setup a proper colormap. -- miod
	 */
	fb->planes = fb->planemask = 1;

	fb->bmv = rbox_windowmove;
	rbox_restore(fb);
	diofb_fbsetup(fb);

	return 0;
}

void
rbox_restore(struct diofb *fb)
{
	volatile struct rboxfb *rb = (struct rboxfb *)fb->regkva;
	u_int i;

	rb_waitbusy(rb);

	rb->regs.id = GRFHWID;		/* trigger reset */
	DELAY(1000);

	rb->regs.interrupt = 0x04;
	rb->video_enable = 0x01;
	rb->drive = 0x01;
	rb->vdrive = 0x0;

	rb->opwen = 0xFF;

	/*
	 * Clear color map
	 */
	rb_waitbusy(fb->regkva);
	for (i = 0; i < 16; i++) {
		*(fb->regkva + 0x63c3 + i*4) = 0x0;
		*(fb->regkva + 0x6403 + i*4) = 0x0;
		*(fb->regkva + 0x6803 + i*4) = 0x0;
		*(fb->regkva + 0x6c03 + i*4) = 0x0;
		*(fb->regkva + 0x73c3 + i*4) = 0x0;
		*(fb->regkva + 0x7403 + i*4) = 0x0;
		*(fb->regkva + 0x7803 + i*4) = 0x0;
		*(fb->regkva + 0x7c03 + i*4) = 0x0;
	}

	rb->rep_rule = RBOX_DUALROP(RR_COPY);

	/*
	 * I cannot figure out how to make the blink planes stop. So, we
	 * must set both colormaps so that when the planes blink, and
	 * the secondary colormap is active, we still get text.
	 */
	CM1RED(fb)[0x00].value = 0x00;
	CM1GRN(fb)[0x00].value = 0x00;
	CM1BLU(fb)[0x00].value = 0x00;
	CM1RED(fb)[0x01].value = 0xFF;
	CM1GRN(fb)[0x01].value = 0xFF;
	CM1BLU(fb)[0x01].value = 0xFF;

	CM2RED(fb)[0x00].value = 0x00;
	CM2GRN(fb)[0x00].value = 0x00;
	CM2BLU(fb)[0x00].value = 0x00;
	CM2RED(fb)[0x01].value = 0xFF;
	CM2GRN(fb)[0x01].value = 0xFF;
	CM2BLU(fb)[0x01].value = 0xFF;

	rb->blink = 0x00;
	rb->write_enable = 0x01;
	rb->opwen = 0x00;

	/* enable display */
	rb->display_enable = 0x01;
}

int
rbox_ioctl(void *v, void *vs, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct diofb *fb = v;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_RBOX;
		return 0;
	case WSDISPLAYIO_SMODE:
		fb->mapmode = *(u_int *)data;
		if (fb->mapmode == WSDISPLAYIO_MODE_EMUL)
			rbox_restore(fb);
		return 0;
	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->width = fb->ri.ri_width;
		wdf->height = fb->ri.ri_height;
		wdf->depth = fb->ri.ri_depth;
		wdf->cmsize = 0;	/* XXX */
		return 0;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = fb->ri.ri_stride;
		return 0;
	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		/* XXX until color support is implemented */
		return EPASSTHROUGH;
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_SVIDEO:
		return EPASSTHROUGH;
	}

	return EPASSTHROUGH;
}

int
rbox_windowmove(struct diofb *fb, uint16_t sx, uint16_t sy,
    uint16_t dx, uint16_t dy, uint16_t cx, uint16_t cy, int16_t rop,
    int16_t planemask)
{
	volatile struct rboxfb *rb = (struct rboxfb *)fb->regkva;

	if (planemask != 0xff)
		return EINVAL;

	rb_waitbusy(rb);

	rb->rep_rule = RBOX_DUALROP(rop);
	rb->source_y = sy;
	rb->source_x = sx;
	rb->dest_y = dy;
	rb->dest_x = dx;
	rb->wheight = cy;
	rb->wwidth  = cx;
	rb->wmove = 1;

	rb_waitbusy(rb);

	return 0;
}

/*
 * Renaissance console support
 */
int
rboxcnattach(bus_space_tag_t bst, bus_addr_t addr, int scode)
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
	    (fbr->id != GRFHWID) || (fbr->fbid != GID_RENAISSANCE)) {
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
	rbox_reset(fb, conscode, (struct diofbreg *)conaddr);

	/*
	 * Initialize the terminal emulator.
	 */
	diofb_cnattach(fb);
	return 0;
}
