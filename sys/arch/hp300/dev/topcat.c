/*	$NetBSD: topcat.c,v 1.6.2.1 2024/05/16 12:27:50 martin Exp $	*/
/*	$OpenBSD: topcat.c,v 1.15 2006/08/11 18:33:13 miod Exp $	*/

/*
 * Copyright (c) 2005, Miodrag Vallat.
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
 * from: Utah $Hdr: grf_tc.c 1.20 93/08/13$
 *
 *	@(#)grf_tc.c	8.4 (Berkeley) 1/12/94
 */

/*
 * Graphics routines for TOPCAT, CATSEYE and KATHMANDU frame buffers
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
#include <hp300/dev/topcatreg.h>

struct	topcat_softc {
	device_t	sc_dev;
	struct diofb	*sc_fb;
	struct diofb	sc_fb_store;
	int		sc_scode;
};

static int	topcat_dio_match(device_t, cfdata_t, void *);
static void	topcat_dio_attach(device_t, device_t, void *);
static int	topcat_intio_match(device_t, cfdata_t, void *);
static void	topcat_intio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(topcat_dio, sizeof(struct topcat_softc),
    topcat_dio_match, topcat_dio_attach, NULL, NULL);

CFATTACH_DECL_NEW(topcat_intio, sizeof(struct topcat_softc),
    topcat_intio_match, topcat_intio_attach, NULL, NULL);

static void	topcat_end_attach(struct topcat_softc *, uint8_t);
static int	topcat_reset(struct diofb *, int, struct diofbreg *);
static void	topcat_restore(struct diofb *);
static int	topcat_setcmap(struct diofb *, struct wsdisplay_cmap *);
static void	topcat_setcolor(struct diofb *, u_int);
static int	topcat_windowmove(struct diofb *, uint16_t, uint16_t, uint16_t,
		    uint16_t, uint16_t, uint16_t, int16_t, int16_t);

static int	topcat_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static void	topcat_putchar8(void *, int, int, u_int, long);
static void	topcat_putchar1_4(void *, int, int, u_int, long);

static struct wsdisplay_accessops topcat_accessops = {
	topcat_ioctl,
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
topcat_intio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	struct diofbreg *fbr;

	if (strcmp("fb", ia->ia_modname) != 0)
		return 0;

	fbr = (struct diofbreg *)ia->ia_addr;

	if (badaddr((void *)fbr))
		return 0;

	if (fbr->id == GRFHWID) {
		switch (fbr->fbid) {
		case GID_TOPCAT:
		case GID_LRCATSEYE:
		case GID_HRCCATSEYE:
		case GID_HRMCATSEYE:
#if 0
		case GID_XXXCATSEYE:
#endif
			return 1;
		}
	}

	return 0;
}

void
topcat_intio_attach(device_t parent, device_t self, void *aux)
{
	struct topcat_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	struct diofbreg *fbr;

	sc->sc_dev = self;
	fbr = (struct diofbreg *)ia->ia_addr;
	sc->sc_scode = CONSCODE_INTERNAL;

	if (sc->sc_scode == conscode) {
		sc->sc_fb = &diofb_cn;
	} else {
		sc->sc_fb = &sc->sc_fb_store;
		topcat_reset(sc->sc_fb, sc->sc_scode, fbr);
	}

	topcat_end_attach(sc, fbr->fbid);
}

int
topcat_dio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct dio_attach_args *da = aux;

	if (da->da_id == DIO_DEVICE_ID_FRAMEBUFFER) {
		switch (da->da_secid) {
		case DIO_DEVICE_SECID_TOPCAT:
		case DIO_DEVICE_SECID_LRCATSEYE:
		case DIO_DEVICE_SECID_HRCCATSEYE:
		case DIO_DEVICE_SECID_HRMCATSEYE:
#if 0
		case DIO_DEVICE_SECID_XXXCATSEYE:
#endif
			return 1;
		}
	}

	return 0;
}

void
topcat_dio_attach(device_t parent, device_t self, void *aux)
{
	struct topcat_softc *sc = device_private(self);
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
		if (topcat_reset(sc->sc_fb, sc->sc_scode, fbr) != 0) {
			aprint_error(": can't reset framebuffer\n");
			return;
		}
	}

	topcat_end_attach(sc, fbr->fbid);
}

void
topcat_end_attach(struct topcat_softc *sc, uint8_t id)
{
	const char *fbname = "unknown";

	switch (id) {
	case GID_TOPCAT:
		switch (sc->sc_fb->planes) {
		case 1:
			if (sc->sc_fb->dheight == 400)
				fbname = "HP98542 topcat";
			else
				fbname = "HP98544 topcat";
			break;
		case 4:
			if (sc->sc_fb->dheight == 400)
				fbname = "HP98543 topcat";
			else
				fbname = "HP98545 topcat";
			break;
		case 6:
			fbname = "HP98547 topcat";
			break;
		}
		break;
	case GID_HRCCATSEYE:
		fbname = "HP98550 catseye";	/* also A1416 kathmandu */
		break;
	case GID_LRCATSEYE:
		fbname = "HP98549 catseye";
		break;
	case GID_HRMCATSEYE:
		fbname = "HP98548 catseye";
		break;
	}

	diofb_end_attach(sc->sc_dev, &topcat_accessops, sc->sc_fb,
	    sc->sc_scode == conscode, fbname);
}

/*
 * Initialize hardware and display routines.
 */
int
topcat_reset(struct diofb *fb, int scode, struct diofbreg *fbr)
{
	volatile struct tcboxfb *tc = (struct tcboxfb *)fbr;
	struct rasops_info *ri = &fb->ri;
	int rc;
	u_int i;
	bool sparse = false;

	if ((rc = diofb_fbinquire(fb, scode, fbr)) != 0)
		return rc;

	/*
	 * If we could not get a valid number of planes, determine it
	 * by writing to the first frame buffer display location,
	 * then reading it back.
	 */
	if (fb->planes == 0) {
		volatile uint8_t *fbp;
		uint8_t save;

		fbp = (uint8_t *)fb->fbkva;
		tc->fben = ~0;
		tc->wen = ~0;
		tc->ren = ~0;
		tc->prr = RR_COPY;
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

	/*
	 * Some displays, such as the HP332 and HP340 internal video
	 * and HP98542/98543 appear to return a display width of 1024
	 * instead of 512. It looks these boards have actually have
	 * enough 64KB (1bpp) or 256KB (4bpp) VRAM and RAMDAC capabilities
	 * to display 1024x400 pixels.
	 * 
	 * However HP's officlal "Service Information Manual" for
	 * "HP 900 Series 300 Computers Models 330/350" says:
	 *  "The medium-resolution board uses eight memory chips per plane.
	 *   This is enough to display 512 doubled pixels by 400 scan lines."
	 *
	 * This "512 doubled pixels" implies that the native HP-UX treated
	 * these 1024x400 framebuffers as pseudo 512x400 ones because
	 * ancient 1980s CRTs (such as 35741) didn't display such higher
	 * resolution. Furthermore, even modern LCDs can only handle
	 * upto 720 pixels in the "400 line" as VGA compatible mode.
	 *
	 * As mentioned above, we treat these 1024x400 1 bit or 4 bit
	 * framebuffers as "2 bytes per pixel" ones, so we have to handle
	 * 512 pixels per line with 1024 bytes per line.
	 */
	if (fb->planes <= 4 && fb->dwidth == 1024 && fb->dheight == 400) {
		fb->dwidth = 512;
		sparse = true;
	}

	fb->bmv = topcat_windowmove;
	topcat_restore(fb);
	diofb_fbsetup(fb);
	if (!sparse) {
		/* save original rasops putchar op */
		fb->wsputchar = ri->ri_ops.putchar;
		ri->ri_ops.putchar = topcat_putchar8;
	} else {
		ri->ri_ops.putchar = topcat_putchar1_4;
		/* copycols and erasecols ops require byte size of fontwidth */
		fb->wsd.fontwidth *= 2;
		/* copyrows and eraserows ops require byte size per line */
		ri->ri_emuwidth *= 2;
	}
	for (i = 0; i <= fb->planemask; i++)
		topcat_setcolor(fb, i);

	return 0;
}

void
topcat_restore(struct diofb *fb)
{
	volatile struct tcboxfb *tc = (struct tcboxfb *)fb->regkva;

	/*
	 * Catseye looks a lot like a topcat, but not completely.
	 * So, we set some bits to make it work.
	 */
	if (tc->regs.fbid != GID_TOPCAT) {
		while ((tc->catseye_status & 1))
			;
		tc->catseye_status = 0x0;
		tc->vb_select = 0x0;
		tc->tcntrl = 0x0;
		tc->acntrl = 0x0;
		tc->pncntrl = 0x0;
		tc->rug_cmdstat = 0x90;
	}

	/*
	 * Enable reading/writing of all the planes.
	 */
	tc->fben = fb->planemask;
	tc->wen  = fb->planemask;
	tc->ren  = fb->planemask;
	tc->prr  = RR_COPY;

	/* Enable display */
	tc->nblank = fb->planemask;
}

int
topcat_ioctl(void *v, void *vs, u_long cmd, void *data, int flags,
    struct lwp *l)
{
	struct diofb *fb = v;
	struct wsdisplay_fbinfo *wdf;
	u_int i;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_TOPCAT;
		return 0;
	case WSDISPLAYIO_SMODE:
		fb->mapmode = *(u_int *)data;
		if (fb->mapmode == WSDISPLAYIO_MODE_EMUL) {
			topcat_restore(fb);
			for (i = 0; i <= fb->planemask; i++)
				topcat_setcolor(fb, i);
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
		if (fb->planemask == 1)
			return EPASSTHROUGH;
		return diofb_getcmap(fb, (struct wsdisplay_cmap *)data);
	case WSDISPLAYIO_PUTCMAP:
		if (fb->planemask == 1)
			return EPASSTHROUGH;
		return topcat_setcmap(fb, (struct wsdisplay_cmap *)data);
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_SVIDEO:
		return EPASSTHROUGH;
	}

	return EPASSTHROUGH;
}

void
topcat_setcolor(struct diofb *fb, u_int index)
{
	volatile struct tcboxfb *tc = (struct tcboxfb *)fb->regkva;

	/* No color map registers on monochrome framebuffers. */
	if (fb->planemask == 1)
		return;

	tc_waitbusy(tc, fb->planemask);

	if (tc->regs.fbid != GID_TOPCAT) {
		tccm_waitbusy(tc);
		tc->plane_mask = fb->planemask;
		tc->cindex = ~index;
		tc->rdata  = fb->cmap.r[index];
		tc->gdata  = fb->cmap.g[index];
		tc->bdata  = fb->cmap.b[index];
		tc->strobe = 0xff;
		/* XXX delay required on 68020/30 to avoid bus error */
		DELAY(100);

		tccm_waitbusy(tc);
		tc->cindex = 0;
	} else {
		tccm_waitbusy(tc);
		tc->plane_mask = fb->planemask;
		tc->rdata  = fb->cmap.r[index];
		tc->gdata  = fb->cmap.g[index];
		tc->bdata  = fb->cmap.b[index];
		DELAY(1);	/* necessary for at least old HP98543 */
		tc->cindex = ~index;
		DELAY(1);	/* necessary for at least old HP98543 */
		tc->strobe = 0xff;
		/* XXX delay required on 68020/30 to avoid bus error */
		DELAY(100);

		tccm_waitbusy(tc);
		tc->rdata  = 0;
		tc->gdata  = 0;
		tc->bdata  = 0;
		tc->cindex = 0;
	}
}

int
topcat_setcmap(struct diofb *fb, struct wsdisplay_cmap *cm)
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
		topcat_setcolor(fb, index++);

	return 0;
}

/*
 * Accelerated routines
 */

int
topcat_windowmove(struct diofb *fb, uint16_t sx, uint16_t sy,
    uint16_t dx, uint16_t dy, uint16_t cx, uint16_t cy, int16_t rop,
    int16_t planemask)
{
	volatile struct tcboxfb *tc = (struct tcboxfb *)fb->regkva;

	tc_waitbusy(tc, fb->planemask);

	if (planemask != 0xff) {
		tc->wen = planemask ^ 0xff;
		tc->wmrr = rop ^ 0x0f;
		tc->wen = fb->planemask;
	} else {
		tc->wen = planemask;
		tc->wmrr = rop;
	}
	tc->source_y = sy;
	tc->source_x = sx;
	tc->dest_y = dy;
	tc->dest_x = dx;
	tc->wheight = cy;
	tc->wwidth = cx;
	tc->wmove = fb->planemask;

	return 0;
}

static void
topcat_putchar8(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct diofb *diofb = ri->ri_hw;
	volatile struct tcboxfb *tc = (struct tcboxfb *)diofb->regkva;

	/* Wait windowmove ops complete before drawing a glyph */
	tc_waitbusy(tc, diofb->planemask);

	/* Call the original rasops putchar */
	(*diofb->wsputchar)(cookie, row, col, uc, attr);
}

/*
 * Put a single character on 1 bpp (98542) or 4 bpp (98543) variants
 * with 1024x400 VRAM to treat them as a pseudo 512x400 bitmap.
 */
static void
topcat_putchar1_4(void *cookie, int row, int col, u_int uc, long attr)
{
	int width, height, cnt, fs;
	uint32_t fb;
	uint8_t *fr, clr[2];
	uint8_t *dp, *rp;
	struct rasops_info *ri;
	struct diofb *diofb;
	volatile struct tcboxfb *tc;

	ri = (struct rasops_info *)cookie;

	if (!CHAR_IN_FONT(uc, ri->ri_font))
		return;

	rp = ri->ri_bits + (row * ri->ri_yscale) +
	    (col * ri->ri_xscale * 2);

	height = ri->ri_font->fontheight;
	width = ri->ri_font->fontwidth;
	clr[0] = (uint8_t)ri->ri_devcmap[(attr >> 16) & 0xf];
	clr[1] = (uint8_t)ri->ri_devcmap[(attr >> 24) & 0xf];

	/* Wait windowmove ops complete before drawing a glyph */
	diofb = ri->ri_hw;
	tc = (struct tcboxfb *)diofb->regkva;
	tc_waitbusy(tc, diofb->planemask);

	/*
	 * We have to put pixel data to both odd and even addresses
	 * to handle "doubled pixels" as noted above.
	 */
	if (uc == ' ') {
		uint16_t c = clr[0];

		c = c << 8 | c;
		while (height--) {
			dp = rp;
			rp += ri->ri_stride;

			for (cnt = width; cnt; cnt--) {
				*(uint16_t *)dp = c;
				dp += 2;
			}
		}
	} else {
		uc -= ri->ri_font->firstchar;
		fr = (uint8_t *)ri->ri_font->data + uc * ri->ri_fontscale;
		fs = ri->ri_font->stride;

		while (height--) {
			dp = rp;
			fb = be32dec(fr);
			fr += fs;
			rp += ri->ri_stride;

			for (cnt = width; cnt; cnt--) {
				uint16_t c = clr[(fb >> 31) & 1];

				c = c << 8 | c;
				*(uint16_t *)dp = c;
				dp += 2;
				fb <<= 1;
			}
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		uint16_t c = clr[1];

		c = c << 8 | c;
		rp -= ri->ri_stride * ri->ri_ul.off;

		while (width--) {
			*(uint16_t *)rp = c;
			rp += 2;
		}
	}
}

/*
 *   Topcat/catseye console attachment
 */

int
topcatcnattach(bus_space_tag_t bst, bus_addr_t addr, int scode)
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

	if (badaddr(va) || fbr->id != GRFHWID) {
		bus_space_unmap(bst, bsh, PAGE_SIZE);
		return 1;
	}

	switch (fbr->fbid) {
	case GID_TOPCAT:
	case GID_LRCATSEYE:
	case GID_HRCCATSEYE:
	case GID_HRMCATSEYE:
		break;

	default:
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
	topcat_reset(fb, conscode, (struct diofbreg *)conaddr);

	/*
	 * Initialize the terminal emulator.
	 */
	diofb_cnattach(fb);
	return 0;
}
