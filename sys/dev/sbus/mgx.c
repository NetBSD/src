/*	$NetBSD: mgx.c,v 1.13 2018/03/28 15:33:44 macallan Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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

/* a console driver for the SSB 4096V-MGX graphics card */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mgx.c,v 1.13 2018/03/28 15:33:44 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/kauth.h>
#include <sys/atomic.h>

#include <sys/bus.h>
#include <machine/autoconf.h>

#include <dev/sbus/sbusvar.h>
#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>

#include <dev/ic/vgareg.h>
#include <dev/sbus/mgxreg.h>

#include "ioconf.h"

#include "opt_wsemul.h"
#include "opt_mgx.h"

struct mgx_softc {
	device_t	sc_dev;
	struct fbdevice	sc_fb;		/* frame buffer device */
	bus_space_tag_t sc_tag;
	bus_space_handle_t sc_blith;
	bus_space_handle_t sc_vgah;
	bus_addr_t	sc_paddr, sc_rpaddr;
	void		*sc_fbaddr;
	uint8_t		*sc_cursor;
	int		sc_width;
	int		sc_height;
	int		sc_stride;
	int		sc_depth;
	int		sc_fbsize;
	int		sc_mode;
	char		sc_name[8];
	uint32_t	sc_dec;
	u_char		sc_cmap_red[256];
	u_char		sc_cmap_green[256];
	u_char		sc_cmap_blue[256];
	int		sc_cursor_x, sc_cursor_y;
	int		sc_hotspot_x, sc_hotspot_y;
	int		sc_video, sc_buf;
	void (*sc_putchar)(void *, int, int, u_int, long);
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
	glyphcache 	sc_gc;
	uint8_t		sc_in[256];
};

static int	mgx_match(device_t, cfdata_t, void *);
static void	mgx_attach(device_t, device_t, void *);
static int	mgx_ioctl(void *, void *, u_long, void *, int,
				 struct lwp*);
static paddr_t	mgx_mmap(void *, void *, off_t, int);
static void	mgx_init_screen(void *, struct vcons_screen *, int,
				 long *);
static void	mgx_write_dac(struct mgx_softc *, int, int, int, int);
static void	mgx_setup(struct mgx_softc *, int);
static void	mgx_init_palette(struct mgx_softc *);
static int	mgx_putcmap(struct mgx_softc *, struct wsdisplay_cmap *);
static int 	mgx_getcmap(struct mgx_softc *, struct wsdisplay_cmap *);
static int	mgx_wait_engine(struct mgx_softc *);
__unused static int	mgx_wait_host(struct mgx_softc *);
/*static*/ int	mgx_wait_fifo(struct mgx_softc *, unsigned int);

static void	mgx_bitblt(void *, int, int, int, int, int, int, int);
static void 	mgx_rectfill(void *, int, int, int, int, long);

static void	mgx_putchar_aa(void *, int, int, u_int, long);
static void	mgx_putchar_mono(void *, int, int, u_int, long);
static void	mgx_cursor(void *, int, int, int);
static void	mgx_copycols(void *, int, int, int, int);
static void	mgx_erasecols(void *, int, int, int, long);
static void	mgx_copyrows(void *, int, int, int);
static void	mgx_eraserows(void *, int, int, long);
static void	mgx_adapt(struct vcons_screen *, void *);

static int	mgx_do_cursor(struct mgx_softc *, struct wsdisplay_cursor *);
static void	mgx_set_cursor(struct mgx_softc *);
static void	mgx_set_video(struct mgx_softc *, int);

CFATTACH_DECL_NEW(mgx, sizeof(struct mgx_softc),
    mgx_match, mgx_attach, NULL, NULL);

struct wsdisplay_accessops mgx_accessops = {
	mgx_ioctl,
	mgx_mmap,
	NULL,	/* vcons_alloc_screen */
	NULL,	/* vcons_free_screen */
	NULL,	/* vcons_show_screen */
	NULL,	/* load_font */
	NULL,	/* polls */
	NULL,	/* scroll */
};

static void	mgx_unblank(device_t);

dev_type_open(mgxopen);
dev_type_close(mgxclose);
dev_type_ioctl(mgxioctl);
dev_type_mmap(mgxmmap);

const struct cdevsw mgx_cdevsw = {
	.d_open = mgxopen,
	.d_close = mgxclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = mgxioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = mgxmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

/* frame buffer generic driver */
static struct fbdriver mgx_fbdriver = {
	mgx_unblank, mgxopen, mgxclose, mgxioctl, nopoll, mgxmmap,
	nokqfilter
};


static inline void
mgx_write_vga(struct mgx_softc *sc, uint32_t reg, uint8_t val)
{
	bus_space_write_1(sc->sc_tag, sc->sc_vgah, reg ^ 3, val);
}

static inline uint8_t
mgx_read_vga(struct mgx_softc *sc, uint32_t reg)
{
	return bus_space_read_1(sc->sc_tag, sc->sc_vgah, reg ^ 3);
}

static inline void
mgx_write_1(struct mgx_softc *sc, uint32_t reg, uint8_t val)
{
	bus_space_write_1(sc->sc_tag, sc->sc_blith, reg ^ 3, val);
}

static inline uint8_t
mgx_read_1(struct mgx_softc *sc, uint32_t reg)
{
	return bus_space_read_1(sc->sc_tag, sc->sc_blith, reg ^ 3);
}

#if 0
static inline uint32_t
mgx_read_4(struct mgx_softc *sc, uint32_t reg)
{
	return bus_space_read_4(sc->sc_tag, sc->sc_blith, reg);
}
#endif

static inline void
mgx_write_2(struct mgx_softc *sc, uint32_t reg, uint16_t val)
{
	bus_space_write_2(sc->sc_tag, sc->sc_blith, reg ^ 2, val);
}

static inline void
mgx_write_4(struct mgx_softc *sc, uint32_t reg, uint32_t val)
{
	bus_space_write_4(sc->sc_tag, sc->sc_blith, reg, val);
}

static int
mgx_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	if (strcmp("SMSI,mgx", sa->sa_name) == 0)
		return 100;
	return 0;
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void
mgx_attach(device_t parent, device_t self, void *args)
{
	struct mgx_softc *sc = device_private(self);
	struct sbus_attach_args *sa = args;
	struct wsemuldisplaydev_attach_args aa;
	struct fbdevice *fb = &sc->sc_fb;
	struct rasops_info *ri;
	unsigned long defattr;
	bus_space_handle_t bh;
	int node = sa->sa_node, bsize;
	int isconsole;

	aprint_normal("\n");
	sc->sc_dev = self;
	sc->sc_tag = sa->sa_bustag;

	sc->sc_paddr = sbus_bus_addr(sa->sa_bustag, sa->sa_slot,
	    sa->sa_reg[8].oa_base);
	sc->sc_rpaddr = sbus_bus_addr(sa->sa_bustag, sa->sa_slot,
	    sa->sa_reg[5].oa_base + MGX_REG_ATREG_OFFSET);

	/* read geometry information from the device tree */
	sc->sc_width = prom_getpropint(sa->sa_node, "width", 1152);
	sc->sc_height = prom_getpropint(sa->sa_node, "height", 900);
	sc->sc_stride = prom_getpropint(sa->sa_node, "linebytes", 1152);
	sc->sc_fbsize = prom_getpropint(sa->sa_node, "fb_size", 0x00400000);
	sc->sc_fbaddr = NULL;
	if (sc->sc_fbaddr == NULL) {
		if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_reg[8].oa_base,
			 sc->sc_fbsize,
			 BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_LARGE,
			 &bh) != 0) {
			aprint_error_dev(self, "couldn't map framebuffer\n");
			return;
		}
		sc->sc_fbaddr = bus_space_vaddr(sa->sa_bustag, bh);
	}

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_reg[4].oa_base, 0x1000, 0,
			 &sc->sc_vgah) != 0) {
		aprint_error("%s: couldn't map VGA registers\n", 
		    device_xname(sc->sc_dev));
		return;
	}

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_reg[5].oa_base + MGX_REG_ATREG_OFFSET, 0x1000,
			 0, &sc->sc_blith) != 0) {
		aprint_error("%s: couldn't map blitter registers\n", 
		    device_xname(sc->sc_dev));
		return;
	}

	mgx_setup(sc, MGX_DEPTH);

	aprint_normal_dev(self, "[%s] %d MB framebuffer, %d x %d\n",
		sc->sc_name, sc->sc_fbsize >> 20, sc->sc_width, sc->sc_height);


	sc->sc_defaultscreen_descr = (struct wsscreen_descr) {
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE |
		WSSCREEN_RESIZE,
		NULL
	};
	
	sc->sc_cursor_x = 0;
	sc->sc_cursor_y = 0;
	sc->sc_hotspot_x = 0;
	sc->sc_hotspot_y = 0;
	sc->sc_video = WSDISPLAYIO_VIDEO_ON;

	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};

	isconsole = fb_is_console(node);

	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	wsfont_init();

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr, &mgx_accessops);
	sc->vd.init_screen = mgx_init_screen;
	sc->vd.show_screen_cookie = sc;
	sc->vd.show_screen_cb = mgx_adapt;

	vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1, &defattr);
	sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	ri = &sc->sc_console_screen.scr_ri;

	sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
	sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
	sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
	sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;

	sc->sc_gc.gc_bitblt = mgx_bitblt;
	sc->sc_gc.gc_rectfill = mgx_rectfill;
	sc->sc_gc.gc_blitcookie = sc;
	sc->sc_gc.gc_rop = ROP_SRC;

	/* 
	 * leave some room between visible screen and glyph cache for upload
	 * buffers used by putchar_mono()
	 */
	bsize = (32 * 1024 * sc->sc_stride - 1) / sc->sc_stride;
	glyphcache_init(&sc->sc_gc,
	    sc->sc_height + bsize,
	    (0x400000 / sc->sc_stride) - (sc->sc_height + bsize),
	    sc->sc_width,
	    ri->ri_font->fontwidth,
	    ri->ri_font->fontheight,
	    defattr);

	mgx_init_palette(sc);

	if(isconsole) {
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	}

	aa.console = isconsole;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &mgx_accessops;
	aa.accesscookie = &sc->vd;

	config_found(self, &aa, wsemuldisplaydevprint);

	/* now the Sun fb goop */
	fb->fb_driver = &mgx_fbdriver;
	fb->fb_device = sc->sc_dev;
	fb->fb_flags = device_cfdata(sc->sc_dev)->cf_flags & FB_USERMASK;
	fb->fb_type.fb_type = FBTYPE_MGX;
	fb->fb_pixels = NULL;

	fb->fb_type.fb_depth = 32;
	fb->fb_type.fb_width = sc->sc_width;
	fb->fb_type.fb_height = sc->sc_height;
	fb->fb_linebytes = sc->sc_stride * 4;

	fb->fb_type.fb_cmsize = 256;
	fb->fb_type.fb_size = sc->sc_fbsize;
	fb_attach(&sc->sc_fb, isconsole);
}

static void
mgx_write_dac(struct mgx_softc *sc, int idx, int r, int g, int b)
{
	mgx_write_vga(sc, VGA_BASE + VGA_DAC_ADDRW, idx);
	mgx_write_vga(sc, VGA_BASE + VGA_DAC_PALETTE, r);
	mgx_write_vga(sc, VGA_BASE + VGA_DAC_PALETTE, g);
	mgx_write_vga(sc, VGA_BASE + VGA_DAC_PALETTE, b);
}

static void
mgx_init_palette(struct mgx_softc *sc)
{
	struct rasops_info *ri = &sc->sc_console_screen.scr_ri;
	int i, j = 0;
	uint8_t cmap[768];

	if (sc->sc_depth == 8) {
		rasops_get_cmap(ri, cmap, sizeof(cmap));
		for (i = 0; i < 256; i++) {
			sc->sc_cmap_red[i] = cmap[j];
			sc->sc_cmap_green[i] = cmap[j + 1];
			sc->sc_cmap_blue[i] = cmap[j + 2];
			mgx_write_dac(sc, i, cmap[j], cmap[j + 1], cmap[j + 2]);
			j += 3;
		}
	} else {
		/* linear ramp for true colour modes */
		for (i = 0; i < 256; i++) {
			mgx_write_dac(sc, i, i, i, i);
		}
	}
}

static int
mgx_putcmap(struct mgx_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	memcpy(&sc->sc_cmap_red[index], &rbuf[index], count);
	memcpy(&sc->sc_cmap_green[index], &gbuf[index], count);
	memcpy(&sc->sc_cmap_blue[index], &bbuf[index], count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];
	
	for (i = 0; i < count; i++) {
		mgx_write_dac(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
mgx_getcmap(struct mgx_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 255 || count > 256 || index + count > 256)
		return EINVAL;
	
	error = copyout(&sc->sc_cmap_red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

static int
mgx_wait_engine(struct mgx_softc *sc)
{
	unsigned int i;
	uint8_t stat;

	for (i = 100000; i != 0; i--) {
		stat = mgx_read_1(sc, ATR_BLT_STATUS);
		if ((stat & (BLT_HOST_BUSY | BLT_ENGINE_BUSY)) == 0)
			break;
	}

	return i;
}

static inline int
mgx_wait_host(struct mgx_softc *sc)
{
	unsigned int i;
	uint8_t stat;

	for (i = 10000; i != 0; i--) {
		stat = mgx_read_1(sc, ATR_BLT_STATUS);
		if ((stat & BLT_HOST_BUSY) == 0)
			break;
	}

	return i;
}

/*static inline*/ int
mgx_wait_fifo(struct mgx_softc *sc, unsigned int nfifo)
{
	unsigned int i;
	uint8_t stat;

	for (i = 100000; i != 0; i--) {
		stat = mgx_read_1(sc, ATR_FIFO_STATUS);
		stat = (stat & FIFO_MASK) >> FIFO_SHIFT;
		if (stat >= nfifo)
			break;
		mgx_write_1(sc, ATR_FIFO_STATUS, 0);
	}

	return i;
}

static void
mgx_setup(struct mgx_softc *sc, int depth)
{
	uint32_t stride;
	int i;
	uint8_t reg;

	/* wait for everything to go idle */
	if (mgx_wait_engine(sc) == 0)
		return;
	if (mgx_wait_fifo(sc, FIFO_AT24) == 0)
		return;

	sc->sc_buf = 0;
	/* read name from sequencer */
	for (i = 0; i < 8; i++) {
		mgx_write_vga(sc, SEQ_INDEX, i + 0x11);
		sc->sc_name[i] = mgx_read_vga(sc, SEQ_DATA);
	}
	sc->sc_name[7] = 0;

	reg = mgx_read_1(sc, ATR_PIXEL);
	reg &= ~PIXEL_DEPTH_MASK;

	switch (depth) {
		case 8:
			sc->sc_dec = DEC_DEPTH_8 << DEC_DEPTH_SHIFT;
			reg |= PIXEL_8;
			break;
		case 15:
			sc->sc_dec = DEC_DEPTH_16 << DEC_DEPTH_SHIFT;
			reg |= PIXEL_15;
			break;
		case 16:
			sc->sc_dec = DEC_DEPTH_16 << DEC_DEPTH_SHIFT;
			reg |= PIXEL_16;
			break;
		case 32:
			sc->sc_dec = DEC_DEPTH_32 << DEC_DEPTH_SHIFT;
			reg |= PIXEL_32;
			break;
		default:
			return; /* not supported */
	}

	/* the chip wants stride in units of 8 bytes */
	sc->sc_stride = sc->sc_width * (depth >> 3);
	stride = sc->sc_stride >> 3;
#ifdef MGX_DEBUG
	sc->sc_height -= 150;
#endif

	sc->sc_depth = depth;

	switch (sc->sc_width) {
		case 640:
			sc->sc_dec |= DEC_WIDTH_640 << DEC_WIDTH_SHIFT;
			break;
		case 800:
			sc->sc_dec |= DEC_WIDTH_800 << DEC_WIDTH_SHIFT;
			break;
		case 1024:
			sc->sc_dec |= DEC_WIDTH_1024 << DEC_WIDTH_SHIFT;
			break;
		case 1152:
			sc->sc_dec |= DEC_WIDTH_1152 << DEC_WIDTH_SHIFT;
			break;
		case 1280:
			sc->sc_dec |= DEC_WIDTH_1280 << DEC_WIDTH_SHIFT;
			break;
		case 1600:
			sc->sc_dec |= DEC_WIDTH_1600 << DEC_WIDTH_SHIFT;
			break;
		default:
			return; /* not supported */
	}
	mgx_wait_fifo(sc, 4);
	mgx_write_1(sc, ATR_CLIP_CONTROL, 0);
	mgx_write_1(sc, ATR_BYTEMASK, 0xff);
	mgx_write_1(sc, ATR_PIXEL, reg);
	mgx_write_4(sc, ATR_OFFSET, 0);
	mgx_wait_fifo(sc, 4);
	mgx_write_vga(sc, CRTC_INDEX, 0x13);
	mgx_write_vga(sc, CRTC_DATA, stride & 0xff);
	mgx_write_vga(sc, CRTC_INDEX, 0x1c);
	mgx_write_vga(sc, CRTC_DATA, (stride & 0xf00) >> 4);

	/* clean up the screen if we're switching to != 8bit */
	if (depth != MGX_DEPTH) 
		mgx_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height, 0);	

	mgx_wait_fifo(sc, 4);
	/* initialize hardware cursor stuff */
	mgx_write_2(sc, ATR_CURSOR_ADDRESS, (sc->sc_fbsize - 1024) >> 10);
	mgx_write_1(sc, ATR_CURSOR_ENABLE, 0);
	sc->sc_cursor = (uint8_t *)sc->sc_fbaddr + sc->sc_fbsize - 1024;
	memset(sc->sc_cursor, 0xf0, 1024);
	memset(sc->sc_in, 0, sizeof(sc->sc_in));
}

static void
mgx_bitblt(void *cookie, int xs, int ys, int xd, int yd, int wi, int he,
             int rop)
{
	struct mgx_softc *sc = cookie;
	uint32_t dec = sc->sc_dec;

        dec |= (DEC_COMMAND_BLT << DEC_COMMAND_SHIFT) |
	       (DEC_START_DIMX << DEC_START_SHIFT);
	if (xs < xd) {
		xs += wi - 1;
		xd += wi - 1;
		dec |= DEC_DIR_X_REVERSE;
	}
	if (ys < yd) {
		ys += he - 1;
		yd += he - 1;
		dec |= DEC_DIR_Y_REVERSE;
	}
	mgx_wait_fifo(sc, 5);
	mgx_write_1(sc, ATR_ROP, rop);
	mgx_write_4(sc, ATR_DEC, dec);
	mgx_write_4(sc, ATR_SRC_XY, (ys << 16) | xs);
	mgx_write_4(sc, ATR_DST_XY, (yd << 16) | xd);
	mgx_write_4(sc, ATR_WH, (he << 16) | wi);
}
	
static void
mgx_rectfill(void *cookie, int x, int y, int wi, int he, long fg)
{
	struct mgx_softc *sc = cookie;
	struct vcons_screen *scr = sc->vd.active;
	uint32_t dec = sc->sc_dec;
	uint32_t col;

	if (scr == NULL)
		return;
	col = scr->scr_ri.ri_devcmap[fg];

	dec = sc->sc_dec;
	dec |= (DEC_COMMAND_RECT << DEC_COMMAND_SHIFT) |
	       (DEC_START_DIMX << DEC_START_SHIFT);
	mgx_wait_fifo(sc, 5);
	mgx_write_1(sc, ATR_ROP, ROP_SRC);
	mgx_write_4(sc, ATR_FG, col);
	mgx_write_4(sc, ATR_DEC, dec);
	mgx_write_4(sc, ATR_DST_XY, (y << 16) | x);
	mgx_write_4(sc, ATR_WH, (he << 16) | wi);
}

static void
mgx_putchar_aa(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct mgx_softc *sc = scr->scr_cookie;
	uint32_t fg, bg;
	int x, y, wi, he, rv;

	wi = font->fontwidth;
	he = font->fontheight;

	bg = (attr >> 16) & 0xf;
	fg = (attr >> 24) & 0xf;

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	if (c == 0x20) {
		mgx_rectfill(sc, x, y, wi, he, bg);
		if (attr & 1)
			mgx_rectfill(sc, x, y + he - 2, wi, 1, fg);
		return;
	}

	rv = glyphcache_try(&sc->sc_gc, c, x, y, attr);
	if (rv != GC_OK) {
		volatile uint32_t junk;

		mgx_wait_engine(sc);
		sc->sc_putchar(cookie, row, col, c, attr & ~1);
		if (rv == GC_ADD) {
			/*
			 * try to make sure the glyph made it all the way to
			 * video memory before trying to blit it into the cache
			 */ 
			junk = *(uint32_t *)sc->sc_fbaddr;
			__USE(junk);
			glyphcache_add(&sc->sc_gc, c, x, y);
		}
	}
	if (attr & 1)
		mgx_rectfill(sc, x, y + he - 2, wi, 1, fg);
}

static void
mgx_putchar_mono(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct mgx_softc *sc = scr->scr_cookie;
	uint8_t *s, *d;
	uint32_t fg, bg, scratch = ((sc->sc_stride * sc->sc_height) + 7) & ~7;
	int x, y, wi, he, len, i;

	wi = font->fontwidth;
	he = font->fontheight;

	bg = (attr >> 16) & 0xf;
	fg = (attr >> 24) & 0xf;

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	if (!CHAR_IN_FONT(c, font)) {
		c = 0x20;
#ifdef MGX_DEBUG
		bg = WSCOL_LIGHT_BLUE;
#endif
	}
	if (c == 0x20) {
		mgx_rectfill(sc, x, y, wi, he, bg);
		if (attr & 1)
			mgx_rectfill(sc, x, y + he - 2, wi, 1, fg);
		return;
	}

	mgx_wait_fifo(sc, 3);
	mgx_write_4(sc, ATR_FG, ri->ri_devcmap[fg]);
	mgx_write_4(sc, ATR_BG, ri->ri_devcmap[bg]);
	mgx_write_1(sc, ATR_ROP, ROP_SRC);

	/*
	 * do hardware colour expansion
	 * there has to be an upload port somewhere, since there are host blit
	 * commands, but it's not used or mentioned in the xf86-video-apm driver
	 * so for now we use the vram-to-vram blits to draw characters.
	 * stash most of the font in vram for speed, also:
	 * - the sbus-pci bridge doesn't seem to support 64bit accesses,
	 *   they will cause occasional data corruption and all sorts of weird
	 *   side effects, so copy font bitmaps byte-wise
	 * - at least it doesn't seem to need any kind of buffer flushing
	 * - still use rotation buffers for characters that don't fall into the
	 *   8 bit range
	 */

	len = (ri->ri_fontscale + 7) & ~7;
	s = WSFONT_GLYPH(c, font);
	if ((c > 32) && (c < 256)) {
		scratch += len * c;
		if (sc->sc_in[c] == 0) {
			d = (uint8_t *)sc->sc_fbaddr + scratch;
			for (i = 0; i < ri->ri_fontscale; i++)
				d[i] = s[i];
			sc->sc_in[c] = 1;
		}
	} else {
		sc->sc_buf = (sc->sc_buf + 1) & 7; /* rotate through 8 buffers */
		scratch += sc->sc_buf * len;
		d = (uint8_t *)sc->sc_fbaddr + scratch;
		for (i = 0; i < ri->ri_fontscale; i++)
			d[i] = s[i];
	}
	mgx_wait_fifo(sc, 5);
	mgx_write_4(sc, ATR_DEC, sc->sc_dec | (DEC_COMMAND_BLT << DEC_COMMAND_SHIFT) |
	       (DEC_START_DIMX << DEC_START_SHIFT) |
	       DEC_SRC_LINEAR | DEC_SRC_CONTIGUOUS | DEC_MONOCHROME);
	mgx_write_4(sc, ATR_SRC_XY, ((scratch & 0xfff000) << 4) | (scratch & 0xfff));
	mgx_write_4(sc, ATR_DST_XY, (y << 16) | x);
	mgx_write_4(sc, ATR_WH, (he << 16) | wi);

	if (attr & 1)
		mgx_rectfill(sc, x, y + he - 2, wi, 1, fg);
}

static void
mgx_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgx_softc *sc = scr->scr_cookie;
	int x, y, wi,he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if (ri->ri_flg & RI_CURSOR) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		mgx_bitblt(sc, x, y, x, y, wi, he, ROP_INV);
		ri->ri_flg &= ~RI_CURSOR;
	}

	ri->ri_crow = row;
	ri->ri_ccol = col;

	if (on)
	{
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		mgx_bitblt(sc, x, y, x, y, wi, he, ROP_INV);
		ri->ri_flg |= RI_CURSOR;
	}
}

static void
mgx_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgx_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;

	xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
	xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
	y = ri->ri_yorigin + ri->ri_font->fontheight * row;
	width = ri->ri_font->fontwidth * ncols;
	height = ri->ri_font->fontheight;
	mgx_bitblt(sc, xs, y, xd, y, width, height, ROP_SRC);
}

static void
mgx_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgx_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, bg;

	x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
	y = ri->ri_yorigin + ri->ri_font->fontheight * row;
	width = ri->ri_font->fontwidth * ncols;
	height = ri->ri_font->fontheight;
	bg = (fillattr >> 16) & 0xff;
	mgx_rectfill(sc, x, y, width, height, bg);
}

static void
mgx_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgx_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	x = ri->ri_xorigin;
	ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
	yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
	width = ri->ri_emuwidth;
	height = ri->ri_font->fontheight * nrows;
	mgx_bitblt(sc, x, ys, x, yd, width, height, ROP_SRC);
}

static void
mgx_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgx_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, bg;

	if ((row == 0) && (nrows == ri->ri_rows)) {
		x = y = 0;
		width = ri->ri_width;
		height = ri->ri_height;
	} else {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
	}
	bg = (fillattr >> 16) & 0xff;
	mgx_rectfill(sc, x, y, width, height, bg);
}

static void
mgx_adapt(struct vcons_screen *scr, void *cookie)
{
	struct mgx_softc *sc = cookie;
	memset(sc->sc_in, 0, sizeof(sc->sc_in));
	glyphcache_adapt(scr, &sc->sc_gc);
}

static void
mgx_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct mgx_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER | RI_ENABLE_ALPHA;

	if (ri->ri_depth == 8)
		ri->ri_flg |= RI_8BIT_IS_RGB;

#ifdef MGX_NOACCEL
	scr->scr_flags |= VCONS_DONT_READ;
#endif
	scr->scr_flags |= VCONS_LOADFONT;

	ri->ri_rnum = 8;
	ri->ri_rpos = 0;
	ri->ri_gnum = 8;
	ri->ri_gpos = 8;
	ri->ri_bnum = 8;
	ri->ri_bpos = 16;

	ri->ri_bits = sc->sc_fbaddr;

	rasops_init(ri, 0, 0);

	ri->ri_caps = WSSCREEN_REVERSE | WSSCREEN_WSCOLORS |
		      WSSCREEN_UNDERLINE | WSSCREEN_RESIZE;

	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
		    ri->ri_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	
#ifdef MGX_NOACCEL
if (0)
#endif
	{
		if (FONT_IS_ALPHA(ri->ri_font)) {
			sc->sc_putchar = ri->ri_ops.putchar;
			ri->ri_ops.putchar   = mgx_putchar_aa;
		} else {
			ri->ri_ops.putchar   = mgx_putchar_mono;
		}
		ri->ri_ops.cursor    = mgx_cursor;
		ri->ri_ops.copyrows  = mgx_copyrows;
		ri->ri_ops.eraserows = mgx_eraserows;
		ri->ri_ops.copycols  = mgx_copycols;
		ri->ri_ops.erasecols = mgx_erasecols;
	}
}

static int
mgx_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data *vd = v;
	struct mgx_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_MGX;
			return 0;

		case WSDISPLAYIO_GINFO:
			wdf = (void *)data;
			wdf->height = sc->sc_height;
			wdf->width = sc->sc_width;
			wdf->depth = 8;
			wdf->cmsize = 256;
			return 0;

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGATTR:
#define fba ((struct fbgattr *)data)
		fba->real_type = sc->sc_fb.fb_type.fb_type;
		fba->owner = 0;		/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = -1;
#undef fba
		break;
		case FBIOGVIDEO:
		case WSDISPLAYIO_GVIDEO:
			*(int *)data = sc->sc_video;
			return 0;

		case WSDISPLAYIO_SVIDEO:
		case FBIOSVIDEO:
			mgx_set_video(sc, *(int *)data);
			return 0;

		case WSDISPLAYIO_LINEBYTES:
			{
				int *ret = (int *)data;
				*ret = sc->sc_stride;
			}
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;
				if (new_mode != sc->sc_mode)
				{
					sc->sc_mode = new_mode;
					if (new_mode == WSDISPLAYIO_MODE_EMUL)
					{
						mgx_setup(sc, MGX_DEPTH);
						glyphcache_wipe(&sc->sc_gc);
						mgx_init_palette(sc);
						vcons_redraw_screen(ms);
					} else {
						mgx_setup(sc, 32);
						mgx_init_palette(sc);
					}
				}
			}
			return 0;

		case WSDISPLAYIO_GETCMAP:
			return mgx_getcmap(sc, (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return mgx_putcmap(sc, (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_GCURPOS:
			{
				struct wsdisplay_curpos *cp = (void *)data;

				cp->x = sc->sc_cursor_x;
				cp->y = sc->sc_cursor_y;
			}
			return 0;

		case WSDISPLAYIO_SCURPOS:
			{
				struct wsdisplay_curpos *cp = (void *)data;

				sc->sc_cursor_x = cp->x;
				sc->sc_cursor_y = cp->y;
				mgx_set_cursor(sc);
			}
			return 0;

		case WSDISPLAYIO_GCURMAX:
			{
				struct wsdisplay_curpos *cp = (void *)data;

				cp->x = 64;
				cp->y = 64;
			}
			return 0;

		case WSDISPLAYIO_SCURSOR:
			{
				struct wsdisplay_cursor *cursor = (void *)data;

				return mgx_do_cursor(sc, cursor);
			}
		case WSDISPLAYIO_GET_FBINFO:
			{
				struct wsdisplayio_fbinfo *fbi = data;
	
				fbi->fbi_fbsize = sc->sc_fbsize - 1024;
				fbi->fbi_width = sc->sc_width;
				fbi->fbi_height = sc->sc_height;
				fbi->fbi_bitsperpixel = sc->sc_depth;
				fbi->fbi_stride = sc->sc_stride;
				fbi->fbi_pixeltype = WSFB_RGB;
				fbi->fbi_subtype.fbi_rgbmasks.red_offset = 8;
				fbi->fbi_subtype.fbi_rgbmasks.red_size = 8;
				fbi->fbi_subtype.fbi_rgbmasks.green_offset = 16;
				fbi->fbi_subtype.fbi_rgbmasks.green_size = 8;
				fbi->fbi_subtype.fbi_rgbmasks.blue_offset = 24;
				fbi->fbi_subtype.fbi_rgbmasks.blue_size = 8;
				fbi->fbi_subtype.fbi_rgbmasks.alpha_offset = 0;
				fbi->fbi_subtype.fbi_rgbmasks.alpha_size = 8;
				return 0;
			}
	}
	return EPASSTHROUGH;
}

static paddr_t
mgx_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct mgx_softc *sc = vd->cookie;

	/* regular fb mapping at 0 */
	if ((offset >= 0) && (offset < sc->sc_fbsize)) {
		return bus_space_mmap(sc->sc_tag, sc->sc_paddr,
		    offset, prot, BUS_SPACE_MAP_LINEAR);
	}

	/*
	 * Blitter registers at 0x80000000, only in mapped mode.
	 * Restrict to root, even though I'm fairly sure the DMA engine lives
	 * elsewhere ( and isn't documented anyway )
	 */
	if (kauth_authorize_machdep(kauth_cred_get(),
	    KAUTH_MACHDEP_UNMANAGEDMEM,
	    NULL, NULL, NULL, NULL) != 0) {
		aprint_normal("%s: mmap() rejected.\n",
		    device_xname(sc->sc_dev));
		return -1;
	}
	if ((sc->sc_mode == WSDISPLAYIO_MODE_MAPPED) &&
	    (offset >= 0x80000000) && (offset < 0x80001000)) {
		return bus_space_mmap(sc->sc_tag, sc->sc_rpaddr,
		    offset, prot, BUS_SPACE_MAP_LINEAR);
	}
	return -1;
}

static int
mgx_do_cursor(struct mgx_softc *sc, struct wsdisplay_cursor *cur)
{
	int i;
	if (cur->which & WSDISPLAY_CURSOR_DOCUR) {

		if (cur->enable) {
			mgx_set_cursor(sc);
			mgx_write_1(sc, ATR_CURSOR_ENABLE, 1);
		} else {
			mgx_write_1(sc, ATR_CURSOR_ENABLE, 0);
		}
	}
	if (cur->which & WSDISPLAY_CURSOR_DOHOT) {

		sc->sc_hotspot_x = cur->hot.x;
		sc->sc_hotspot_y = cur->hot.y;
		mgx_set_cursor(sc);
	}
	if (cur->which & WSDISPLAY_CURSOR_DOPOS) {

		sc->sc_cursor_x = cur->pos.x;
		sc->sc_cursor_y = cur->pos.y;
		mgx_set_cursor(sc);
	}
	if (cur->which & WSDISPLAY_CURSOR_DOCMAP) {
		int cnt = min(2, cur->cmap.count);
		uint8_t c;
		uint8_t r[2], g[2], b[2];

		copyin(cur->cmap.red, r, cnt);
		copyin(cur->cmap.green, g, cnt);
		copyin(cur->cmap.blue, b, cnt);
	
		for (i = 0; i < cnt; i++) {
			c = r[i] & 0xe0;
			c |= (g[i] & 0xe0) >> 3;
			c |= (b[i] & 0xc0) >> 6;
			mgx_write_1(sc, ATR_CURSOR_FG + i, c);
		}
	}
	if (cur->which & WSDISPLAY_CURSOR_DOSHAPE) {
		int j;
		uint8_t *fb = sc->sc_cursor;
		uint8_t temp;
		uint8_t im, ma, px;

		for (i = 0; i < 512; i++) {
			temp = 0;
			copyin(&cur->image[i], &im, 1);
			copyin(&cur->mask[i], &ma, 1);
			for (j = 0; j < 4; j++) {
				temp >>= 2;
				px = (ma & 1) ? 0 : 0x80;
				if (px == 0)
					px |= (im & 1) ? 0 : 0x40;
				temp |= px;
				im >>= 1;
				ma >>= 1;
			}
			*fb = temp;
			fb++;
			temp = 0;
			for (j = 0; j < 4; j++) {
				temp >>= 2;
				px = (ma & 1) ? 0 : 0x80;
				if (px == 0)
					px |= (im & 1) ? 0 : 0x40;
				temp |= px;
				im >>= 1;
				ma >>= 1;
			}
			*fb = temp;
			fb++;
		}
	}
	return 0;
}

static void
mgx_set_cursor(struct mgx_softc *sc)
{
	uint32_t reg;
	uint16_t hot;

	reg = (sc->sc_cursor_y << 16) | (sc->sc_cursor_x & 0xffff);
	mgx_write_4(sc, ATR_CURSOR_POSITION, reg);
	hot = (sc->sc_hotspot_y << 8) | (sc->sc_hotspot_x & 0xff);
	mgx_write_2(sc, ATR_CURSOR_HOTSPOT, hot);
}

static void
mgx_set_video(struct mgx_softc *sc, int v)
{
	uint8_t reg;

	if (sc->sc_video == v)
		return;

	sc->sc_video = v;
	reg = mgx_read_1(sc, ATR_DPMS);

	if (v == WSDISPLAYIO_VIDEO_ON) {
		reg &= ~DPMS_SYNC_DISABLE_ALL;
	} else {
		reg |= DPMS_SYNC_DISABLE_ALL;
	}
	mgx_write_1(sc, ATR_DPMS, reg);
}

/* Sun fb dev goop */
static void
mgx_unblank(device_t dev)
{
	struct mgx_softc *sc = device_private(dev);

	mgx_set_video(sc, WSDISPLAYIO_VIDEO_ON);
}

paddr_t
mgxmmap(dev_t dev, off_t offset, int prot)
{
	struct mgx_softc *sc = device_lookup_private(&mgx_cd, minor(dev));

	/* regular fb mapping at 0 */
	if ((offset >= 0) && (offset < sc->sc_fbsize)) {
		return bus_space_mmap(sc->sc_tag, sc->sc_paddr,
		    offset, prot, BUS_SPACE_MAP_LINEAR);
	}

	/*
	 * Blitter registers at 0x80000000, only in mapped mode.
	 * Restrict to root, even though I'm fairly sure the DMA engine lives
	 * elsewhere ( and isn't documented anyway )
	 */
	if (kauth_authorize_machdep(kauth_cred_get(),
	    KAUTH_MACHDEP_UNMANAGEDMEM,
	    NULL, NULL, NULL, NULL) != 0) {
		aprint_normal("%s: mmap() rejected.\n",
		    device_xname(sc->sc_dev));
		return -1;
	}
	if ((sc->sc_mode == WSDISPLAYIO_MODE_MAPPED) &&
	    (offset >= 0x80000000) && (offset < 0x80001000)) {
		return bus_space_mmap(sc->sc_tag, sc->sc_rpaddr,
		    offset, prot, BUS_SPACE_MAP_LINEAR);
	}
	return -1;
}

int
mgxopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	device_t dv = device_lookup(&mgx_cd, minor(dev));
	struct mgx_softc *sc = device_private(dv);

	if (dv == NULL)
		return ENXIO;
	if (sc->sc_mode == WSDISPLAYIO_MODE_MAPPED)
		return 0;
	sc->sc_mode = WSDISPLAYIO_MODE_MAPPED;
	mgx_setup(sc, 32);
	mgx_init_palette(sc);
	return 0;
}

int
mgxclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	device_t dv = device_lookup(&mgx_cd, minor(dev));
	struct mgx_softc *sc = device_private(dv);
	struct vcons_screen *ms = sc->vd.active;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)
		return 0;

	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;

	mgx_setup(sc, MGX_DEPTH);
	glyphcache_wipe(&sc->sc_gc);
	mgx_init_palette(sc);
	if (ms != NULL) {
		vcons_redraw_screen(ms);
	}
	return 0;
}

int
mgxioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct mgx_softc *sc = device_lookup_private(&mgx_cd, minor(dev));

	return mgx_ioctl(&sc->vd, NULL, cmd, data, flags, l);
}
