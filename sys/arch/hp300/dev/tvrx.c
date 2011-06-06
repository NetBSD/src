/*	$NetBSD: tvrx.c,v 1.3.6.2 2011/06/06 09:05:37 jruoho Exp $	*/
/*	$OpenBSD: tvrx.c,v 1.1 2006/04/14 21:05:43 miod Exp $	*/

/*
 * Copyright (c) 2006, Miodrag Vallat.
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
 *
 */

/*
 * Graphics routines for the TurboVRX frame buffer
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

#include <dev/cons.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <hp300/dev/diofbreg.h>
#include <hp300/dev/diofbvar.h>

struct	tvrx_softc {
	device_t	sc_dev;
	struct diofb	*sc_fb;
	struct diofb	sc_fb_store;
	int		sc_scode;
};

static int	tvrx_match(device_t, cfdata_t, void *);
static void	tvrx_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tvrx, sizeof(struct tvrx_softc),
    tvrx_match, tvrx_attach, NULL, NULL);

static int	tvrx_reset(struct diofb *, int, struct diofbreg *);

static int	tvrx_ioctl(void *, void *, u_long, void *, int, struct lwp *);

static struct wsdisplay_accessops tvrx_accessops = {
	tvrx_ioctl,
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
tvrx_match(device_t parent, cfdata_t cf, void *aux)
{
	struct dio_attach_args *da = aux;

	if (da->da_id != DIO_DEVICE_ID_FRAMEBUFFER ||
	    da->da_secid != DIO_DEVICE_SECID_TIGERSHARK)
		return 0;

	return 1;
}

void
tvrx_attach(device_t parent, device_t self, void *aux)
{
	struct tvrx_softc *sc = device_private(self);
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
		if (bus_space_map(da->da_bst, da->da_addr, da->da_size, 0,
		    &bsh)) {
			aprint_error(": can't map framebuffer\n");
			return;
		}
		fbr = bus_space_vaddr(da->da_bst, bsh);
		if (tvrx_reset(sc->sc_fb, sc->sc_scode, fbr) != 0) {
			aprint_error(": can't reset framebuffer\n");
			return;
		}
	}

	diofb_end_attach(self, &tvrx_accessops, sc->sc_fb,
	    sc->sc_scode == conscode, NULL);
}

/*
 * Initialize hardware and display routines.
 */
int
tvrx_reset(struct diofb *fb, int scode, struct diofbreg *fbr)
{
	int rc;

	if ((rc = diofb_fbinquire(fb, scode, fbr)) != 0)
		return rc;

	/*
	 * We rely on the PROM to initialize the frame buffer in the mode
	 * we expect it: cleared, overlay plane enabled and accessible
	 * at the beginning of the video memory.
	 *
	 * This is NOT the mode we would end up by simply resetting the
	 * board.
	 */

	fb->ri.ri_depth = 1;
	fb->bmv = diofb_mono_windowmove;
	diofb_fbsetup(fb);

	return 0;
}

int
tvrx_ioctl(void *v, void *vs, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct diofb *fb = v;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_TVRX;
		return 0;
	case WSDISPLAYIO_SMODE:
		fb->mapmode = *(u_int *)data;
		return 0;
	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->width = fb->ri.ri_width;
		wdf->height = fb->ri.ri_height;
		wdf->depth = fb->ri.ri_depth;
		wdf->cmsize = 0;
		return 0;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = fb->ri.ri_stride;
		return 0;
	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		/* until color support is implemented */
		return EPASSTHROUGH;
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_SVIDEO:
		/* unsupported */
		return EPASSTHROUGH;
	}

	return EPASSTHROUGH;
}

/*
 * Console support
 */

int
tvrxcnattach(bus_space_tag_t bst, bus_addr_t addr, int scode)
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
	    fbr->id != GRFHWID || fbr->fbid == GID_TIGER) {
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
	tvrx_reset(fb, conscode, (struct diofbreg *)conaddr);

	/*
	 * Initialize the terminal emulator.
	 */
	diofb_cnattach(fb);
	return 0;
}
