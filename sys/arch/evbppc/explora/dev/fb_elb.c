/*	$NetBSD: fb_elb.c,v 1.20 2021/08/07 16:18:52 thorpej Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fb_elb.c,v 1.20 2021/08/07 16:18:52 thorpej Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <machine/explora.h>
#include <sys/bus.h>

#include <evbppc/explora/dev/elbvar.h>

#define FB_NPORTS		65536
#define CMAP_SIZE		256

struct fb_dev {
	void *fb_vram;
	bus_space_tag_t fb_iot;
	bus_space_handle_t fb_ioh;
	struct rasops_info fb_ri;
};

struct fb_cmap {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct fb_elb_softc {
	device_t sc_dev;
	struct fb_dev *sc_fb;
	bus_addr_t sc_fbbase;
	bus_size_t sc_fbsize;
	int sc_nscreens;
	int sc_mode;
	struct fb_cmap sc_cmap[CMAP_SIZE];
};

/*
 * We assume that rasops_cmap is compatible to sc_cmap.
 */
CTASSERT(sizeof(rasops_cmap) == CMAP_SIZE * 3);

void		fb_cnattach(bus_space_tag_t, bus_addr_t, void *);

static int	fb_elb_probe(device_t, cfdata_t, void *);
static void	fb_elb_attach(device_t, device_t, void *);

static void	fb_init(struct fb_dev *, int, int);
static void	fb_initcmap(struct fb_elb_softc *);

static int	fb_getcmap(struct fb_elb_softc *, struct wsdisplay_cmap *);
static int	fb_putcmap(struct fb_elb_softc *,
		    const struct wsdisplay_cmap *);

static int	fb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	fb_mmap(void *, void *, off_t, int);
static int	fb_alloc_screen(void *, const struct wsscreen_descr *, void **,
                    int *, int *, long *);
static void	fb_free_screen(void *, void *);
static int	fb_show_screen(void *, void *, int, void (*)(void *, int, int),
                    void *);

static void	fb_eraserows(void *, int, int, long);
static void	fb_erasecols(void *, int, int, int, long);
static void	fb_copyrows(void *, int, int, int);
static void	fb_copycols(void *, int, int, int, int);

static void	s3_getgeometry(struct fb_dev *, int *, int *);
static void	s3_putcmap(struct fb_dev *, const struct fb_cmap *);
static void	s3_copy(struct fb_dev *, int, int, int, int, int, int, int);
static void	s3_fill(struct fb_dev *, int, int, int, int, int, int);

static struct fb_dev console_dev;

static struct wsdisplay_accessops accessops = {
	fb_ioctl,
	fb_mmap,
	fb_alloc_screen,
	fb_free_screen,
	fb_show_screen,
	NULL
};

static struct wsscreen_descr stdscreen = {
	"std",
	0, 0,
	0,
	0, 0,
	0
};

static const struct wsscreen_descr *scrlist[] = {
	&stdscreen
};

static struct wsscreen_list screenlist = {
	__arraycount(scrlist), scrlist
};

void
fb_cnattach(bus_space_tag_t iot, bus_addr_t iobase, void *vram)
{
	struct rasops_info *ri = &console_dev.fb_ri;
	long defattr;
	int width, height;

	console_dev.fb_iot = iot;
	console_dev.fb_ioh = iobase;
	console_dev.fb_vram = vram;

	s3_getgeometry(&console_dev, &width, &height);

	fb_init(&console_dev, width, height);

	s3_putcmap(&console_dev, (const struct fb_cmap*)rasops_cmap);

	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);

	wsdisplay_cnattach(&stdscreen, ri, 0, 0, defattr);
}

CFATTACH_DECL_NEW(fb_elb, sizeof(struct fb_elb_softc),
    fb_elb_probe, fb_elb_attach, NULL, NULL);

static int
fb_elb_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct elb_attach_args *oaa = aux;

	if (strcmp(oaa->elb_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

static void
fb_elb_attach(device_t parent, device_t self, void *aux)
{
	struct fb_elb_softc *sc = device_private(self);
	struct elb_attach_args *eaa = aux;
	struct wsemuldisplaydev_attach_args waa;
	bus_space_handle_t ioh;
	int is_console, width, height;

	sc->sc_dev = self;

	is_console = ((void *)eaa->elb_base == console_dev.fb_vram);

	if (is_console)
		sc->sc_fb = &console_dev;
	else
		sc->sc_fb = kmem_zalloc(sizeof(struct fb_dev), KM_SLEEP);

	sc->sc_fb->fb_iot = eaa->elb_bt;
	bus_space_map(sc->sc_fb->fb_iot, eaa->elb_base2, FB_NPORTS,
	    0, &sc->sc_fb->fb_ioh);

	s3_getgeometry(sc->sc_fb, &width, &height);

	sc->sc_fbbase = eaa->elb_base;
	sc->sc_fbsize = width * height;
	bus_space_map(sc->sc_fb->fb_iot, sc->sc_fbbase, sc->sc_fbsize,
	    BUS_SPACE_MAP_LINEAR, &ioh);
	sc->sc_fb->fb_vram = bus_space_vaddr(sc->sc_fb->fb_iot, ioh);

	if (!is_console)
		fb_init(sc->sc_fb, width, height);
	fb_initcmap(sc);

	printf(": %dx%d 8bpp\n", width, height);

	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	waa.console = is_console;
	waa.scrdata = &screenlist;
	waa.accessops = &accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint, CFARGS_NONE);
}

static void
fb_init(struct fb_dev *fb, int width, int height)
{
	struct rasops_info *ri = &fb->fb_ri;

	ri->ri_width = width;
	ri->ri_height = height;
	ri->ri_depth = 8;
	ri->ri_stride = ri->ri_width;
	ri->ri_bits = fb->fb_vram;
	ri->ri_flg = RI_CENTER | RI_CLEAR;
	if (ri == &console_dev.fb_ri)
		ri->ri_flg |= RI_NO_AUTO;

	rasops_init(ri, 500, 500);

	/* Replace the copy/erase ops. */
	ri->ri_hw = fb;
	ri->ri_ops.eraserows = fb_eraserows;
	ri->ri_ops.erasecols = fb_erasecols;
	ri->ri_ops.copyrows = fb_copyrows;
	ri->ri_ops.copycols = fb_copycols;

	stdscreen.nrows = ri->ri_rows;
	stdscreen.ncols = ri->ri_cols; 
	stdscreen.textops = &ri->ri_ops;
	stdscreen.capabilities = ri->ri_caps;
}

static void
fb_initcmap(struct fb_elb_softc *sc)
{

	memcpy(&sc->sc_cmap, rasops_cmap, sizeof(sc->sc_cmap));
	s3_putcmap(sc->sc_fb, sc->sc_cmap);
}

static int
fb_getcmap(struct fb_elb_softc *sc, struct wsdisplay_cmap *p)
{
	u_int index = p->index, count = p->count;
	uint8_t buf[CMAP_SIZE * 3];
	int error, i;

	if (index >= CMAP_SIZE || count > CMAP_SIZE - index)
		return EINVAL;

	for (i = 0; i < count; i++) {
		buf[i]             = sc->sc_cmap[index + i].r;
		buf[i + count]     = sc->sc_cmap[index + i].g;
		buf[i + 2 * count] = sc->sc_cmap[index + i].b;
	}

	if ((error = copyout(&buf[0],         p->red,   count)) != 0 ||
	    (error = copyout(&buf[count],     p->green, count)) != 0 ||
	    (error = copyout(&buf[2 * count], p->blue,  count)) != 0)
		return error;

	return 0;
}

static int
fb_putcmap(struct fb_elb_softc *sc, const struct wsdisplay_cmap *p)
{
	u_int index = p->index, count = p->count;
	uint8_t buf[CMAP_SIZE * 3];
	int error, i;

	if (index >= CMAP_SIZE || count > CMAP_SIZE - index)
		return EINVAL;

	if ((error = copyin(p->red,   &buf[0],         count)) != 0 ||
	    (error = copyin(p->green, &buf[count],     count)) != 0 ||
	    (error = copyin(p->blue,  &buf[2 * count], count)) != 0)
		return error;

	for (i = 0; i < count; i++) {
		sc->sc_cmap[index + i].r = buf[i];
		sc->sc_cmap[index + i].g = buf[i + count];
		sc->sc_cmap[index + i].b = buf[i + 2 * count];
	}

	s3_putcmap(sc->sc_fb, sc->sc_cmap);

	return 0;
}

static int
fb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct fb_elb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_fb->fb_ri;
	struct wsdisplay_fbinfo *wdf;
	int new_mode;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_UNKNOWN;	/* XXX */
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = ri->ri_height;
		wdf->width = ri->ri_width;
		wdf->depth = ri->ri_depth;
		wdf->cmsize = CMAP_SIZE;
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = ri->ri_stride;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return fb_getcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return fb_putcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_SMODE:
		new_mode = *(int *)data;
		if (new_mode != sc->sc_mode) {
			sc->sc_mode = new_mode;
			if (new_mode == WSDISPLAYIO_MODE_EMUL) {
				/* XXX */
				memset(sc->sc_fb->fb_vram, 0, sc->sc_fbsize);

				fb_initcmap(sc);
			}
		}
		return 0;

	default:
		break;
	}

	return EPASSTHROUGH;
}

static paddr_t
fb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct fb_elb_softc *sc = v;

	if (offset < 0 || offset >= sc->sc_fbsize)
		return -1;

	return bus_space_mmap(sc->sc_fb->fb_iot, sc->sc_fbbase, offset, prot,
	    BUS_SPACE_MAP_LINEAR);
}

static int
fb_alloc_screen(void *v, const struct wsscreen_descr *scrdesc, void **cookiep,
    int *ccolp, int *crowp, long *attrp)
{
	struct fb_elb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_fb->fb_ri;

	if (sc->sc_nscreens > 0)
		return ENOMEM;

	*cookiep = ri;
	*ccolp = *crowp = 0;
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, attrp);
	sc->sc_nscreens++;

	return 0;
}

static void
fb_free_screen(void *v, void *cookie)
{
	struct fb_elb_softc *sc = v;

	if (sc->sc_fb == &console_dev)
		panic("fb_free_screen: freeing console");

	sc->sc_nscreens--;
}

static int
fb_show_screen(void *v, void *cookie, int waitok, void (*cb)(void *, int, int),
    void *cbarg)
{

	return 0;
}

static void
fb_eraserows(void *v, int row, int nrows, long attr)
{
	struct rasops_info *ri = v;
	struct fb_dev *fb = ri->ri_hw;

	row *= ri->ri_font->fontheight; 
	nrows *= ri->ri_font->fontheight;

	s3_fill(fb, 0, row, ri->ri_stride, nrows, (attr >> 16)&0x0f, 0x0f);
}

static void
fb_erasecols(void *v, int row, int startcol, int ncols, long attr)
{
	struct rasops_info *ri = v;
	struct fb_dev *fb = ri->ri_hw;

	row *= ri->ri_font->fontheight; 
	startcol *= ri->ri_font->fontwidth;
	ncols *= ri->ri_font->fontwidth;

	s3_fill(fb, startcol, row, ncols, ri->ri_font->fontheight,
	    (attr >> 16)&0x0f, 0x0f);
}

static void
fb_copyrows(void *v, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = v;
	struct fb_dev *fb = ri->ri_hw;

	srcrow *= ri->ri_font->fontheight;
	dstrow *= ri->ri_font->fontheight;
	nrows *= ri->ri_font->fontheight;

	s3_copy(fb, 0, srcrow, 0, dstrow, ri->ri_stride, nrows, 0x0f);
}

static void
fb_copycols(void *v, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = v;
	struct fb_dev *fb = ri->ri_hw;

	row *= ri->ri_font->fontheight;
	srccol *= ri->ri_font->fontwidth;
	dstcol *= ri->ri_font->fontwidth;
	ncols *= ri->ri_font->fontwidth;

	s3_copy(fb, srccol, row, dstcol, row,
	    ncols, ri->ri_font->fontheight, 0x0f);
}

/*
 * S3 support routines
 */

#define S3_CRTC_INDEX		0x83d4
#define S3_CRTC_DATA		0x83d5

#define S3_DAC_RD_INDEX		0x83c7
#define S3_DAC_WR_INDEX		0x83c8
#define S3_DAC_DATA		0x83c9

#define S3_CUR_Y		0x82e8
#define S3_CUR_X		0x86e8
#define S3_DESTY_AXSTP		0x8ae8
#define S3_DESTX_DIASTP		0x8ee8
#define S3_MAJ_AXIS_PCNT	0x96e8
#define S3_GP_STAT		0x9ae8
#define S3_CMD			0x9ae8
#define S3_BKGD_COLOR		0xa2e8
#define S3_FRGD_COLOR		0xa6e8
#define S3_WRT_MASK		0xaae8
#define S3_RD_MASK		0xaee8
#define S3_BKGD_MIX		0xb6e8
#define S3_FRGD_MIX		0xbae8
#define S3_MULTIFUNC_CNTL	0xbee8

#define S3_GP_STAT_FIFO_1	0x0080
#define S3_GP_STAT_BSY		0x0200

#define S3_CMD_BITBLT		0xc001
#define S3_CMD_RECT		0x4001
#define S3_INC_Y		0x0080
#define S3_INC_X		0x0020 
#define S3_DRAW			0x0010
#define S3_MULTI		0x0002

#define S3_CSRC_BKGDCOL		0x0000
#define S3_CSRC_FRGDCOL		0x0020
#define S3_CSRC_DISPMEM		0x0060
#define S3_MIX_NEW		0x0007

static void
s3_getgeometry(struct fb_dev *fb, int *width, int *height)
{
	bus_space_tag_t iot = fb->fb_iot;
	bus_space_handle_t ioh = fb->fb_ioh;
	int w, h, i;

	bus_space_write_1(iot, ioh, S3_CRTC_INDEX, 1);
	w = bus_space_read_1(iot, ioh, S3_CRTC_DATA);

	bus_space_write_1(iot, ioh, S3_CRTC_INDEX, 18);
	h = bus_space_read_1(iot, ioh, S3_CRTC_DATA);

	bus_space_write_1(iot, ioh, S3_CRTC_INDEX, 7);
	i = bus_space_read_1(iot, ioh, S3_CRTC_DATA);

	h += (i << 7) & 0x100;
	h += (i << 3) & 0x200;

	*width = (w+1) << 3;
	*height = h+1;
}

static void
s3_putcmap(struct fb_dev *fb, const struct fb_cmap *cmap)
{
	bus_space_tag_t iot = fb->fb_iot;
	bus_space_handle_t ioh = fb->fb_ioh;
	int i;

	bus_space_write_1(iot, ioh, S3_DAC_WR_INDEX, 0);
	for (i = 0; i < CMAP_SIZE; i++) {
		bus_space_write_1(iot, ioh, S3_DAC_DATA, cmap[i].r >> 2);
		bus_space_write_1(iot, ioh, S3_DAC_DATA, cmap[i].g >> 2);
		bus_space_write_1(iot, ioh, S3_DAC_DATA, cmap[i].b >> 2);
	}
}

static void
s3_copy(struct fb_dev *fb, int src_x, int src_y, int dest_x, int dest_y,
    int width, int height, int mask)
{
	bus_space_tag_t iot = fb->fb_iot;
	bus_space_handle_t ioh = fb->fb_ioh;
	u_int16_t cmd = S3_CMD_BITBLT | S3_DRAW;

	if (src_x > dest_x)
		cmd |= S3_INC_X;
	else {
		src_x += width-1;
		dest_x += width-1;
	}

	if (src_y > dest_y)
		cmd |= S3_INC_Y;
	else {
		src_y += height-1;
		dest_y += height-1;
	}

	while (bus_space_read_2(iot, ioh, S3_GP_STAT) & S3_GP_STAT_FIFO_1)
		;

	bus_space_write_2(iot, ioh, S3_FRGD_MIX, S3_CSRC_DISPMEM | S3_MIX_NEW);
	bus_space_write_2(iot, ioh, S3_WRT_MASK, mask);
	bus_space_write_2(iot, ioh, S3_CUR_X, src_x);
	bus_space_write_2(iot, ioh, S3_CUR_Y, src_y);
	bus_space_write_2(iot, ioh, S3_DESTX_DIASTP, dest_x);
	bus_space_write_2(iot, ioh, S3_DESTY_AXSTP, dest_y);
	bus_space_write_2(iot, ioh, S3_MULTIFUNC_CNTL, height-1);
	bus_space_write_2(iot, ioh, S3_MAJ_AXIS_PCNT, width-1);
	bus_space_write_2(iot, ioh, S3_CMD, cmd);

	while (bus_space_read_2(iot, ioh, S3_GP_STAT) & S3_GP_STAT_BSY)
		;
}

static void
s3_fill(struct fb_dev *fb, int x, int y, int width, int height,
    int color, int mask)
{
	bus_space_tag_t iot = fb->fb_iot;
	bus_space_handle_t ioh = fb->fb_ioh;
	u_int16_t cmd = S3_CMD_RECT | S3_INC_X | S3_INC_Y | S3_DRAW;

	while (bus_space_read_2(iot, ioh, S3_GP_STAT) & S3_GP_STAT_FIFO_1)
		;

	bus_space_write_2(iot, ioh, S3_FRGD_MIX, S3_CSRC_FRGDCOL | S3_MIX_NEW);
	bus_space_write_2(iot, ioh, S3_FRGD_COLOR, color);
	bus_space_write_2(iot, ioh, S3_WRT_MASK, mask);
	bus_space_write_2(iot, ioh, S3_CUR_X, x);
	bus_space_write_2(iot, ioh, S3_CUR_Y, y);
	bus_space_write_2(iot, ioh, S3_MULTIFUNC_CNTL, height-1);
	bus_space_write_2(iot, ioh, S3_MAJ_AXIS_PCNT, width-1);
	bus_space_write_2(iot, ioh, S3_CMD, cmd);

	while (bus_space_read_2(iot, ioh, S3_GP_STAT) & S3_GP_STAT_BSY)
		;
}
