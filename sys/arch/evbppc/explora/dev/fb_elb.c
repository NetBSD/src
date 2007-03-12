/*	$NetBSD: fb_elb.c,v 1.7.14.1 2007/03/12 05:47:39 rmind Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fb_elb.c,v 1.7.14.1 2007/03/12 05:47:39 rmind Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <machine/explora.h>
#include <machine/bus.h>

#include <evbppc/explora/dev/elbvar.h>

#define FB_NPORTS		65536

struct fb_dev {
	void *fb_vram;
	bus_space_tag_t fb_iot;
	bus_space_handle_t fb_ioh;
	struct rasops_info fb_ri;
};

struct fb_elb_softc {
	struct device sc_dev;
	struct fb_dev *sc_fb;
	int sc_nscreens;
};

static int	fb_elb_probe(struct device *, struct cfdata *, void *);
static void	fb_elb_attach(struct device *, struct device *, void *);
void		fb_cnattach(bus_space_tag_t, bus_addr_t, void *);
static void	fb_init(struct fb_dev *, int);
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

static void	s3_init(struct fb_dev *, int *, int *);
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
	sizeof(scrlist)/sizeof(scrlist[0]), scrlist
};

CFATTACH_DECL(fb_elb, sizeof(struct fb_elb_softc),
    fb_elb_probe, fb_elb_attach, NULL, NULL);

static int
fb_elb_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct elb_attach_args *oaa = aux;

	if (strcmp(oaa->elb_name, cf->cf_name) != 0)
		return 0;

	return (1);
}

static void
fb_elb_attach(struct device *parent, struct device *self, void *aux)
{
	struct fb_elb_softc *sc = (void *)self;
	struct elb_attach_args *eaa = aux;
	struct wsemuldisplaydev_attach_args waa;
	struct rasops_info *ri;
	bus_space_handle_t ioh;
	int is_console;

	is_console = ((void *)eaa->elb_base == console_dev.fb_vram);

	if (is_console) {
		sc->sc_fb = &console_dev;
	} else {
		sc->sc_fb = malloc(sizeof(struct fb_dev), M_DEVBUF, M_WAITOK);
		memset(sc->sc_fb, 0, sizeof(struct fb_dev));
	}

	sc->sc_fb->fb_iot = eaa->elb_bt;
	bus_space_map(sc->sc_fb->fb_iot, eaa->elb_base, SIZE_FB,
	    BUS_SPACE_MAP_LINEAR, &ioh);
	sc->sc_fb->fb_vram = bus_space_vaddr(sc->sc_fb->fb_iot, ioh);
	bus_space_map(sc->sc_fb->fb_iot, eaa->elb_base2, FB_NPORTS,
	    0, &sc->sc_fb->fb_ioh);

	fb_init(sc->sc_fb, !is_console);

	ri = &sc->sc_fb->fb_ri;

	printf(": %d x %d\n", ri->ri_rows, ri->ri_cols);

	waa.console = is_console;
	waa.scrdata = &screenlist;
	waa.accessops = &accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

static void
fb_init(struct fb_dev *fb, int full)
{
	struct rasops_info *ri = &fb->fb_ri;

	if (full) {
		s3_init(fb, &ri->ri_width, &ri->ri_height);
		ri->ri_depth = 8;
		ri->ri_stride = ri->ri_width;
		ri->ri_bits = fb->fb_vram;
		ri->ri_flg = RI_CENTER;

		rasops_init(ri, 500, 500);
	} else {
		ri->ri_origbits = fb->fb_vram;	/*XXX*/
		rasops_reconfig(ri, 500, 500);
	}

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

static int
fb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct fb_elb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_fb->fb_ri;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_UNKNOWN;	/* XXX */
		return(0);

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = ri->ri_height;
		wdf->width = ri->ri_width;
		wdf->depth = ri->ri_depth;
		wdf->cmsize = 16; /*XXX*/
		return(0);

	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		break;
	}

	return(EPASSTHROUGH);
}

static paddr_t
fb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct fb_elb_softc *sc = v;

	if (offset < 0 || offset >= SIZE_FB)
		return -1;

	return bus_space_mmap(sc->sc_fb->fb_iot, BASE_FB, offset, prot,
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

	return(0);
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
	return(0);
}

void
fb_cnattach(bus_space_tag_t iot, bus_addr_t iobase, void *vram)
{
	struct rasops_info *ri = &console_dev.fb_ri;
	long defattr;

	console_dev.fb_iot = iot;
	console_dev.fb_ioh = iobase;
	console_dev.fb_vram = vram;

	fb_init(&console_dev, 1);

	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);

	wsdisplay_cnattach(&stdscreen, ri, 0, 0, defattr);
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

#define CMAP_SIZE		256

static u_int8_t default_cmap[] = {
	/* black */		  0,   0,   0,
	/* blue */		  0,   0, 192,
	/* green */		  0, 192,   0,
	/* cyan */		  0, 192, 192,
	/* red */		192,   0,   0,
	/* magenta */		192,   0, 192,
	/* brown */		192, 192,   0,
	/* lightgrey */		212, 208, 200,
	/* darkgrey */		200, 192, 188,
	/* lightblue */		  0,   0, 255,
	/* lightgreen */	  0, 255,   0,
	/* lightcyan */		  0, 255, 255,
	/* lightred */		255,   0,   0,
	/* lightmagenta */	255,   0, 255,
	/* yellow */		255, 255,   0,
	/* white */		255, 255, 255,
};

static void
s3_init(struct fb_dev *fb, int *width, int *height)
{
	int i, j, w, h;
	bus_space_tag_t iot = fb->fb_iot;
	bus_space_handle_t ioh = fb->fb_ioh;

	/* Initialize colormap */

	bus_space_write_1(iot, ioh, S3_DAC_WR_INDEX, 0);

	for (i = j = 0; i < CMAP_SIZE*3; i++) {
		bus_space_write_1(iot, ioh, S3_DAC_DATA, default_cmap[j] >> 2);
		j = (j+1) % sizeof(default_cmap)/sizeof(default_cmap[0]);
	}

	/* Retrieve frame buffer geometry */

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
