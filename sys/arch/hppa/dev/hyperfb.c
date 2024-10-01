/*	$NetBSD: hyperfb.c,v 1.17 2024/10/01 07:44:22 macallan Exp $	*/

/*
 * Copyright (c) 2024 Michael Lorenz
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * a native driver for HCRX / hyperdrive cards
 * tested on a HCRX24Z in a C360 only so far
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hyperfb.c,v 1.17 2024/10/01 07:44:22 macallan Exp $");

#include "opt_cputype.h"
#include "opt_hyperfb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>

#include <dev/ic/stireg.h>
#include <dev/ic/stivar.h>

#include <hppa/dev/cpudevs.h>
#include <hppa/hppa/machdep.h>

#ifdef HYPERFB_DEBUG
#define	DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif

#define	STI_ROMSIZE	(sizeof(struct sti_dd) * 4)

#define HCRX_FBOFFSET	0x01000000
#define HCRX_FBLEN	0x01000000
#define HCRX_REGOFFSET	0x00100000
#define HCRX_REGLEN	0x00280000

#define HCRX_CONFIG_24BIT	0x100

int hyperfb_match(device_t, cfdata_t, void *);
void hyperfb_attach(device_t, device_t, void *);

struct	hyperfb_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_addr_t		sc_base;
	bus_space_handle_t	sc_hfb, sc_hreg;
	void 			*sc_fb;

	int sc_width, sc_height;
	int sc_locked, sc_is_console, sc_24bit;
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
	int sc_mode;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	kmutex_t sc_hwlock;
	uint32_t sc_hwmode;
#define HW_FB		0
#define HW_FILL		1
#define HW_BLIT		2
#define HW_SFILL	3
	/* cursor stuff */
	int sc_cursor_x, sc_cursor_y;
	int sc_hot_x, sc_hot_y, sc_enabled;
	int sc_video_on;
	glyphcache sc_gc;
};

extern struct cfdriver hyperfb_cd;

CFATTACH_DECL_NEW(hyperfb, sizeof(struct hyperfb_softc), hyperfb_match,
    hyperfb_attach, NULL, NULL);

static inline void  hyperfb_setup_fb(struct hyperfb_softc *);
static inline void  hyperfb_setup_fb24(struct hyperfb_softc *);
static void 	hyperfb_init_screen(void *, struct vcons_screen *,
			    int, long *);
static int	hyperfb_ioctl(void *, void *, u_long, void *, int,
			     struct lwp *);
static paddr_t	hyperfb_mmap(void *, void *, off_t, int);

static int	hyperfb_putcmap(struct hyperfb_softc *,
		    struct wsdisplay_cmap *);
static int 	hyperfb_getcmap(struct hyperfb_softc *,
		    struct wsdisplay_cmap *);
static void	hyperfb_restore_palette(struct hyperfb_softc *);
static int 	hyperfb_putpalreg(struct hyperfb_softc *, uint8_t, uint8_t,
			    uint8_t, uint8_t);
void 	hyperfb_setup(struct hyperfb_softc *);
static void	hyperfb_set_video(struct hyperfb_softc *, int);

static void	hyperfb_rectfill(struct hyperfb_softc *, int, int, int, int,
			    uint32_t);
static void	hyperfb_bitblt(void *, int, int, int, int, int,
			    int, int);

static void	hyperfb_cursor(void *, int, int, int);
static void	hyperfb_putchar(void *, int, int, u_int, long);
static void	hyperfb_copycols(void *, int, int, int, int);
static void	hyperfb_erasecols(void *, int, int, int, long);
static void	hyperfb_copyrows(void *, int, int, int);
static void	hyperfb_eraserows(void *, int, int, long);

static void	hyperfb_move_cursor(struct hyperfb_softc *, int, int);
static int	hyperfb_do_cursor(struct hyperfb_softc *,
		    struct wsdisplay_cursor *);

static inline void hyperfb_wait_fifo(struct hyperfb_softc *, uint32_t);

#define	ngle_bt458_write(sc, r, v) \
	hyperfb_write4(sc, NGLE_REG_RAMDAC + ((r) << 2), (v) << 24)

struct wsdisplay_accessops hyperfb_accessops = {
	hyperfb_ioctl,
	hyperfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

static inline uint32_t
hyperfb_read4(struct hyperfb_softc *sc, uint32_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_hreg, offset);
}

static inline uint8_t
hyperfb_read1(struct hyperfb_softc *sc, uint32_t offset)
{
	return bus_space_read_1(sc->sc_iot, sc->sc_hreg, offset);
}

static inline void
hyperfb_write4(struct hyperfb_softc *sc, uint32_t offset, uint32_t val)
{
	bus_space_write_4(sc->sc_iot, sc->sc_hreg, offset, val);
}

static inline void
hyperfb_write1(struct hyperfb_softc *sc, uint32_t offset, uint8_t val)
{
	bus_space_write_1(sc->sc_iot, sc->sc_hreg, offset, val);
}

static inline void
hyperfb_wait(struct hyperfb_softc *sc)
{
	uint8_t stat;

	do {
		stat = hyperfb_read1(sc, NGLE_REG_15b0);
		if (stat == 0)
			stat = hyperfb_read1(sc, NGLE_REG_15b0);
	} while (stat != 0);
}

static inline void
hyperfb_wait_fifo(struct hyperfb_softc *sc, uint32_t slots)
{
	uint32_t reg;

	do {
		reg = hyperfb_read4(sc, NGLE_REG_34);
	} while (reg < slots);
}

static inline void
hyperfb_setup_fb(struct hyperfb_softc *sc)
{

	/*
	 * turns out the plane mask is applied to everything, including
	 * direct framebuffer writes, so make sure we always set it
	 */
	hyperfb_wait(sc);
	if ((sc->sc_mode != WSDISPLAYIO_MODE_EMUL) && sc->sc_24bit) {
		hyperfb_write4(sc, NGLE_REG_10,
		    BA(FractDcd, Otc24, Ots08, AddrLong, 0, BINapp0F8, 0));
		hyperfb_write4(sc, NGLE_REG_13, 0xffffffff);
	} else {
		hyperfb_write4(sc, NGLE_REG_10,
		    BA(IndexedDcd, Otc04, Ots08, AddrByte, 0, BINovly, 0));
		hyperfb_write4(sc, NGLE_REG_13, 0xff);
	}
	hyperfb_write4(sc, NGLE_REG_14, 0x83000300);
	hyperfb_wait(sc);
	hyperfb_write1(sc, NGLE_REG_16b1, 1);
	sc->sc_hwmode = HW_FB;
}

static inline void
hyperfb_setup_fb24(struct hyperfb_softc *sc)
{

	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_10,
	    BA(FractDcd, Otc24, Ots08, AddrLong, 0, BINapp0F8, 0));
	hyperfb_write4(sc, NGLE_REG_13, 0xffffffff);
	hyperfb_write4(sc, NGLE_REG_14, 0x83000300);
	//IBOvals(RopSrc,0,BitmapExtent08,0,DataDynamic,MaskDynamic,0,0)
	hyperfb_wait(sc);
	hyperfb_write1(sc, NGLE_REG_16b1, 1);
	sc->sc_hwmode = HW_FB;
}

int
hyperfb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	bus_space_handle_t romh;
	paddr_t rom;
	uint32_t id = 0;
	u_char devtype;
	int rv = 0, romunmapped = 0;

	if (ca->ca_type.iodc_type != HPPA_TYPE_FIO)
		return 0;

	/* these need further checking for the graphics id */
	if (ca->ca_type.iodc_sv_model != HPPA_FIO_GSGC &&
	    ca->ca_type.iodc_sv_model != HPPA_FIO_SGC)
		return 0;

	if (ca->ca_naddrs > 0)
		rom = ca->ca_addrs[0].addr;
	else
		rom = ca->ca_hpa;

	DPRINTF("%s: hpa=%x, rom=%x\n", __func__, (uint)ca->ca_hpa,
	    (uint)rom);

	/* if it does not map, probably part of the lasi space */
	if (bus_space_map(ca->ca_iot, rom, STI_ROMSIZE, 0, &romh)) {
		DPRINTF("%s: can't map rom space (%d)\n", __func__, rv);

		if ((rom & HPPA_IOBEGIN) == HPPA_IOBEGIN) {
			romh = rom;
			romunmapped++;
		} else {
			/* in this case nobody has no freaking idea */
			return 0;
		}
	}

	devtype = bus_space_read_1(ca->ca_iot, romh, 3);
	DPRINTF("%s: devtype=%d\n", __func__, devtype);
	rv = 1;
	switch (devtype) {
	case STI_DEVTYPE4:
		id = bus_space_read_4(ca->ca_iot, romh, STI_DEV4_DD_GRID);
		break;
	case STI_DEVTYPE1:
		id = (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_GRID
		    +  3) << 24) |
		    (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_GRID
		    +  7) << 16) |
		    (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_GRID
		    + 11) <<  8) |
		    (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_GRID
		    + 15));
		break;
	default:
		DPRINTF("%s: unknown type (%x)\n", __func__, devtype);
		rv = 0;
	}

	if (id == STI_DD_HCRX)
		rv = 100;	/* beat out sti */

	ca->ca_addrs[ca->ca_naddrs].addr = rom;
	ca->ca_addrs[ca->ca_naddrs].size = sti_rom_size(ca->ca_iot, romh);
	ca->ca_naddrs++;

	if (!romunmapped)
		bus_space_unmap(ca->ca_iot, romh, STI_ROMSIZE);
	return rv;
}

void
hyperfb_attach(device_t parent, device_t self, void *aux)
{
	struct hyperfb_softc *sc = device_private(self);
	struct confargs *ca = aux;
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args aa;
	bus_space_handle_t hrom;
	hppa_hpa_t consaddr;
	long defattr;
	int pagezero_cookie;
	paddr_t rom;
	uint32_t config;

	pagezero_cookie = hppa_pagezero_map();
	consaddr = (hppa_hpa_t)PAGE0->mem_cons.pz_hpa;
	hppa_pagezero_unmap(pagezero_cookie);

	sc->sc_dev = self;
	sc->sc_base = ca->ca_hpa;
	sc->sc_iot = ca->ca_iot;
	sc->sc_is_console =(ca->ca_hpa == consaddr);
	sc->sc_width = 1280;
	sc->sc_height = 1024;

	/* we can *not* be interrupted when doing colour map accesses */
	mutex_init(&sc->sc_hwlock, MUTEX_DEFAULT, IPL_HIGH);

	/* we stashed rom addr/len into the last slot during probe */
	rom = ca->ca_addrs[ca->ca_naddrs - 1].addr;

	if (bus_space_map(sc->sc_iot,
	    sc->sc_base + HCRX_FBOFFSET, HCRX_FBLEN,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE,
	    &sc->sc_hfb)) {
	    	aprint_error_dev(sc->sc_dev,
		    "failed to map the framebuffer\n");
	    	return;
	}
	sc->sc_fb = bus_space_vaddr(sc->sc_iot, sc->sc_hfb);

	if (bus_space_map(sc->sc_iot,
	    sc->sc_base + HCRX_REGOFFSET, HCRX_REGLEN, 0, &sc->sc_hreg)) {
	    	aprint_error_dev(sc->sc_dev, "failed to map registers\n");
	    	return;
	}

	/*
	 * we really only need the first word so we can grab the config bits
	 * between the bytes
	 */
	if (bus_space_map(sc->sc_iot, rom, 4, 0, &hrom)) {
	    	aprint_error_dev(sc->sc_dev,
		    "failed to map ROM, assuming 8bit\n");
	    	config = 0;
	} else {
		/* alright, we got the ROM. now do the idle dance. */
		volatile uint32_t r = hyperfb_read4(sc, NGLE_REG_15);
		__USE(r);
		hyperfb_wait(sc);
		config = bus_space_read_4(sc->sc_iot, hrom, 0);
		bus_space_unmap(sc->sc_iot, hrom, 4);
	}
	sc->sc_24bit = ((config & HCRX_CONFIG_24BIT) != 0);

	printf(" %s\n", sc->sc_24bit ? "HCRX24" : "HCRX");
#ifdef HP7300LC_CPU
	/*
	 * PCXL2: enable accel I/O for this space, see PCX-L2 ERS "ACCEL_IO".
	 * "pcxl2_ers.{ps,pdf}", (section / chapter . rel. page / abs. page)
	 * 8.7.4 / 8-12 / 92, 11.3.14 / 11-14 / 122 and 14.8 / 14-5 / 203.
	 */
	if (hppa_cpu_info->hci_cputype == hpcxl2
	    && ca->ca_hpa >= PCXL2_ACCEL_IO_START
	    && ca->ca_hpa <= PCXL2_ACCEL_IO_END)
		eaio_l2(PCXL2_ACCEL_IO_ADDR2MASK(ca->ca_hpa));
#endif /* HP7300LC_CPU */

	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_locked = 0;

	hyperfb_setup(sc);
	hyperfb_setup_fb(sc);

	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE |
		      WSSCREEN_RESIZE,
		NULL
	};

	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &hyperfb_accessops);
	sc->vd.init_screen = hyperfb_init_screen;
	sc->vd.show_screen_cookie = &sc->sc_gc;
	sc->vd.show_screen_cb = glyphcache_adapt;

	ri = &sc->sc_console_screen.scr_ri;

	//sc->sc_gc.gc_bitblt = hyperfb_bitblt;
	//sc->sc_gc.gc_blitcookie = sc;
	//sc->sc_gc.gc_rop = RopSrc;

	if (sc->sc_is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;

#if 0
		glyphcache_init(&sc->sc_gc, sc->sc_height + 5,
				sc->sc_scr.fbheight - sc->sc_height - 5,
				sc->sc_scr.fbwidth,
				ri->ri_font->fontwidth,
				ri->ri_font->fontheight,
				defattr);
#endif
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);

		hyperfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
		    ri->ri_devcmap[(defattr >> 16) & 0xff]);

		vcons_replay_msgbuf(&sc->sc_console_screen);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		if (sc->sc_console_screen.scr_ri.ri_rows == 0) {
			/* do some minimal setup to avoid weirdnesses later */
			vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
			    &defattr);
		} else
			(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);

#if 0
		glyphcache_init(&sc->sc_gc, sc->sc_height + 5,
				sc->sc_scr.fbheight - sc->sc_height - 5,
				sc->sc_scr.fbwidth,
				ri->ri_font->fontwidth,
				ri->ri_font->fontheight,
				defattr);
#endif
	}

	hyperfb_restore_palette(sc);

	/* no suspend/resume support yet */
	if (!pmf_device_register(sc->sc_dev, NULL, NULL))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	aa.console = sc->sc_is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &hyperfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint, CFARGS_NONE);

	hyperfb_setup_fb(sc);
	
#ifdef HYPERFB_DEBUG
	int i;

	hyperfb_wait_fifo(sc, 4);
	/* transfer data */
	hyperfb_write4(sc, NGLE_REG_8, 0xff00ff00);
	/* plane mask */
	hyperfb_write4(sc, NGLE_REG_13, 0xff);
	/* bitmap op */
	hyperfb_write4(sc, NGLE_REG_14,
	    IBOvals(RopSrc, 0, BitmapExtent08, 0, DataDynamic, MaskOtc, 1, 0));
	/* dst bitmap access */
	hyperfb_write4(sc, NGLE_REG_11,
	    BA(IndexedDcd, Otc32, OtsIndirect, AddrLong, 0, BINovly, 0));

	hyperfb_wait_fifo(sc, 3);
	hyperfb_write4(sc, NGLE_REG_35, 0xe0);
	hyperfb_write4(sc, NGLE_REG_36, 0x1c);
	/* dst XY */
	hyperfb_write4(sc, NGLE_REG_6, (2 << 16) | 902);
	/* len XY start */
	hyperfb_write4(sc, NGLE_REG_9, (28 << 16) | 32);

	for (i = 0; i < 32; i++)
		hyperfb_write4(sc, NGLE_REG_8, (i & 4) ? 0xff00ff00 : 0x00ff00ff);

	hyperfb_rectfill(sc, 70, 902, 16, 32, 0xe0);
	hyperfb_rectfill(sc, 50, 902, 16, 32, 0x1c);
#endif
}

static void
hyperfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct hyperfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 8;
	ri->ri_width = 1280;
#ifdef HYPERFB_DEBUG
	ri->ri_height = 900;
#else
	ri->ri_height = 1024;
#endif
	ri->ri_stride = 2048;
	ri->ri_flg = RI_CENTER | RI_8BIT_IS_RGB /*|
		     RI_ENABLE_ALPHA | RI_PREFER_ALPHA*/;

	ri->ri_bits = (void *)sc->sc_fb;
	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE |
		      WSSCREEN_RESIZE;
	scr->scr_flags |= VCONS_LOADFONT;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;

	ri->ri_ops.copyrows = hyperfb_copyrows;
	ri->ri_ops.copycols = hyperfb_copycols;
	ri->ri_ops.eraserows = hyperfb_eraserows;
	ri->ri_ops.erasecols = hyperfb_erasecols;
	ri->ri_ops.cursor = hyperfb_cursor;
	ri->ri_ops.putchar = hyperfb_putchar;
}

static int
hyperfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data *vd = v;
	struct hyperfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_STI;
		return 0;

	case GCID:
		*(u_int *)data = STI_DD_HCRX;
		return 0;

	case WSDISPLAYIO_GINFO:
		if (ms == NULL)
			return ENODEV;
		wdf = (void *)data;
		wdf->height = ms->scr_ri.ri_height;
		wdf->width = sc->sc_24bit ? ms->scr_ri.ri_width << 2
		    : ms->scr_ri.ri_width;
		wdf->depth = ms->scr_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return hyperfb_getcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return hyperfb_putcmap(sc,
		    (struct wsdisplay_cmap *)data);
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_24bit ? 8192 : 2048;
		return 0;

	case WSDISPLAYIO_SMODE: {
		int new_mode = *(int*)data;
		if (new_mode != sc->sc_mode) {
			sc->sc_mode = new_mode;
			if (new_mode == WSDISPLAYIO_MODE_EMUL) {
				hyperfb_setup(sc);
				hyperfb_restore_palette(sc);
#if 0
				glyphcache_wipe(&sc->sc_gc);
#endif
				hyperfb_rectfill(sc, 0, 0, sc->sc_width,
				    sc->sc_height, ms->scr_ri.ri_devcmap[
				    (ms->scr_defattr >> 16) & 0xff]);
				vcons_redraw_screen(ms);
				hyperfb_set_video(sc, 1);
			} else {
				hyperfb_setup(sc);
				hyperfb_rectfill(sc, 0, 0, sc->sc_width,
				    sc->sc_height, 0xff);
				hyperfb_setup_fb24(sc);
			}
		}
		}
		return 0;

	case WSDISPLAYIO_GET_FBINFO: {
		struct wsdisplayio_fbinfo *fbi = data;
		int ret;

		ret = wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
		fbi->fbi_fbsize = sc->sc_height * 2048;
		if (sc->sc_24bit) {
			fbi->fbi_stride = 8192;
			fbi->fbi_bitsperpixel = 32;
			fbi->fbi_pixeltype = WSFB_RGB;
			fbi->fbi_subtype.fbi_rgbmasks.red_offset = 16;
			fbi->fbi_subtype.fbi_rgbmasks.red_size = 8;
			fbi->fbi_subtype.fbi_rgbmasks.green_offset = 8;
			fbi->fbi_subtype.fbi_rgbmasks.green_size = 8;
			fbi->fbi_subtype.fbi_rgbmasks.blue_offset = 0;
			fbi->fbi_subtype.fbi_rgbmasks.blue_size = 8;
			fbi->fbi_subtype.fbi_rgbmasks.alpha_size = 0;
				fbi->fbi_fbsize = sc->sc_height * 8192;
		}
		return ret;
	}

	case WSDISPLAYIO_GCURPOS: {
		struct wsdisplay_curpos *cp = (void *)data;

		cp->x = sc->sc_cursor_x;
		cp->y = sc->sc_cursor_y;
	}
		return 0;

	case WSDISPLAYIO_SCURPOS: {
		struct wsdisplay_curpos *cp = (void *)data;

		hyperfb_move_cursor(sc, cp->x, cp->y);
	}
		return 0;

	case WSDISPLAYIO_GCURMAX: {
		struct wsdisplay_curpos *cp = (void *)data;

		cp->x = 64;
		cp->y = 64;
	}
		return 0;

	case WSDISPLAYIO_SCURSOR: {
		struct wsdisplay_cursor *cursor = (void *)data;

		return hyperfb_do_cursor(sc, cursor);
	}

	case WSDISPLAYIO_SVIDEO:
		hyperfb_set_video(sc, *(int *)data);
		return 0;
	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = sc->sc_video_on ?
		    WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;
		return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
hyperfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct hyperfb_softc *sc = vd->cookie;
	paddr_t pa = -1;


	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)
		return -1;

	/* GSC framebuffer space is 16MB */
	if (offset >= 0 && offset < 0x1000000) {
		/* framebuffer */
		pa = bus_space_mmap(sc->sc_iot, sc->sc_base + HCRX_FBOFFSET,
		    offset, prot,
		    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
	} else if (offset >= 0x80000000 && offset < 0x8040000) {
		/* blitter registers etc. */
		pa = bus_space_mmap(sc->sc_iot, sc->sc_base + HCRX_REGOFFSET,
		    offset - 0x80000000, prot, BUS_SPACE_MAP_LINEAR);
	}

	return pa;
}

static int
hyperfb_putcmap(struct hyperfb_softc *sc, struct wsdisplay_cmap *cm)
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
		hyperfb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
hyperfb_getcmap(struct hyperfb_softc *sc, struct wsdisplay_cmap *cm)
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

static void
hyperfb_restore_palette(struct hyperfb_softc *sc)
{
	uint8_t cmap[768];
	int i, j;

	j = 0;
	rasops_get_cmap(&sc->sc_console_screen.scr_ri, cmap, sizeof(cmap));
	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i] = cmap[j];
		sc->sc_cmap_green[i] = cmap[j + 1];
		sc->sc_cmap_blue[i] = cmap[j + 2];
		hyperfb_putpalreg(sc, i, cmap[j], cmap[j + 1], cmap[j + 2]);
		j += 3;
	}
}

static int
hyperfb_putpalreg(struct hyperfb_softc *sc, uint8_t idx, uint8_t r, uint8_t g,
    uint8_t b)
{

	mutex_enter(&sc->sc_hwlock);
	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_10, 0xbbe0f000);
	hyperfb_write4(sc, NGLE_REG_14, 0x03000300);
	hyperfb_write4(sc, NGLE_REG_13, 0xffffffff);

	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_3, 0x400 | (idx << 2));
	hyperfb_write4(sc, NGLE_REG_4, (r << 16) | (g << 8) | b);

	hyperfb_write4(sc, NGLE_REG_2, 0x400);
	hyperfb_write4(sc, NGLE_REG_38, 0x82000100);
	hyperfb_setup_fb(sc);
	mutex_exit(&sc->sc_hwlock);
	return 0;
}

void
hyperfb_setup(struct hyperfb_softc *sc)
{
	int i;
	uint32_t reg;

	sc->sc_hwmode = HW_FB;
	sc->sc_hot_x = 0;
	sc->sc_hot_y = 0;
	sc->sc_enabled = 0;
	sc->sc_video_on = 1;

	/* set Bt458 read mask register to all planes */
	/* XXX I'm not sure HCRX even has one of these */
	hyperfb_wait(sc);
	ngle_bt458_write(sc, 0x08, 0x04);
	ngle_bt458_write(sc, 0x0a, 0xff);

	reg = hyperfb_read4(sc, NGLE_REG_32);
	DPRINTF("planereg %08x\n", reg);
	hyperfb_write4(sc, NGLE_REG_32, 0xffffffff);

	/* hyperbowl */
	hyperfb_wait(sc);
	if (sc->sc_24bit) {
		/* write must happen twice because hw bug */
		hyperfb_write4(sc, NGLE_REG_40,
		    HYPERBOWL_MODE01_8_24_LUT0_TRANSPARENT_LUT1_OPAQUE);
		hyperfb_write4(sc, NGLE_REG_40,
		    HYPERBOWL_MODE01_8_24_LUT0_TRANSPARENT_LUT1_OPAQUE);
		hyperfb_write4(sc, NGLE_REG_39, HYPERBOWL_MODE2_8_24);
		/* Set lut 0 to be the direct color */
		hyperfb_write4(sc, NGLE_REG_42, 0x014c0148);
		hyperfb_write4(sc, NGLE_REG_43, 0x404c4048);
		hyperfb_write4(sc, NGLE_REG_44, 0x034c0348);
		hyperfb_write4(sc, NGLE_REG_45, 0x444c4448);
	} else {
		hyperfb_write4(sc, NGLE_REG_40,
		    HYPERBOWL_MODE_FOR_8_OVER_88_LUT0_NO_TRANSPARENCIES);
		hyperfb_write4(sc, NGLE_REG_40,
		    HYPERBOWL_MODE_FOR_8_OVER_88_LUT0_NO_TRANSPARENCIES);

		hyperfb_write4(sc, NGLE_REG_42, 0);
		hyperfb_write4(sc, NGLE_REG_43, 0);
		hyperfb_write4(sc, NGLE_REG_44, 0);
		hyperfb_write4(sc, NGLE_REG_45, 0x444c4048);
	}

	/* attr. planes */
	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_11,
	    BA(IndexedDcd, Otc32, OtsIndirect, AddrLong, 0, BINattr, 0));
	hyperfb_write4(sc, NGLE_REG_14,
	    IBOvals(RopSrc, 0, BitmapExtent08, 1, DataDynamic, MaskOtc, 1, 0));
	hyperfb_write4(sc, NGLE_REG_12, 0x04000F00/*NGLE_BUFF0_CMAP0*/);
	hyperfb_write4(sc, NGLE_REG_8, 0xffffffff);

	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_6, 0x00000000);
	hyperfb_write4(sc, NGLE_REG_9,
	    (sc->sc_width << 16) | sc->sc_height);
	/*
	 * blit into offscreen memory to force flush previous - apparently
	 * some chips have a bug this works around
	 */
	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_6, 0x05000000);
	hyperfb_write4(sc, NGLE_REG_9, 0x00040001);

	/*
	 * on 24bit-capable hardware we:
	 * - make overlay colour 255 transparent
	 * - blit the 24bit buffer all white
	 * - install a linear ramp in CMAP 0
	 */
	if (sc->sc_24bit) {
		/* overlay transparency */
		hyperfb_wait_fifo(sc, 7);
		hyperfb_write4(sc, NGLE_REG_11,
		    BA(IndexedDcd, Otc04, Ots08, AddrLong, 0, BINovly, 0));
		hyperfb_write4(sc, NGLE_REG_14, 0x03000300);
		hyperfb_write4(sc, NGLE_REG_3, 0x000017f0);
		hyperfb_write4(sc, NGLE_REG_13, 0xffffffff);
		hyperfb_write4(sc, NGLE_REG_22, 0xffffffff);
		hyperfb_write4(sc, NGLE_REG_23, 0x0);

		hyperfb_wait(sc);
		hyperfb_write4(sc, NGLE_REG_12, 0x00000000);

		/* clear 24bit buffer */
		hyperfb_wait(sc);
		/* plane mask */
		hyperfb_write4(sc, NGLE_REG_13, 0xffffffff);
		hyperfb_write4(sc, NGLE_REG_8, 0xffffffff); /* transfer data */
		/* bitmap op */
		hyperfb_write4(sc, NGLE_REG_14,
		    IBOvals(RopSrc, 0, BitmapExtent32, 0, DataDynamic, MaskOtc,
			0, 0));
		/* dst bitmap access */
		hyperfb_write4(sc, NGLE_REG_11,
		    BA(IndexedDcd, Otc32, OtsIndirect, AddrLong, 0, BINapp0F8,
			0));
		hyperfb_wait_fifo(sc, 3);
		hyperfb_write4(sc, NGLE_REG_35, 0x00ffffff);	/* fg colour */
		hyperfb_write4(sc, NGLE_REG_6, 0x00000000);	/* dst xy */
		hyperfb_write4(sc, NGLE_REG_9,
		    (sc->sc_width << 16) | sc->sc_height);

		/* write a linear ramp into CMAP0 */
		hyperfb_wait(sc);
		hyperfb_write4(sc, NGLE_REG_10, 0xbbe0f000);
		hyperfb_write4(sc, NGLE_REG_14, 0x03000300);
		hyperfb_write4(sc, NGLE_REG_13, 0xffffffff);

		hyperfb_wait(sc);
		hyperfb_write4(sc, NGLE_REG_3, 0);
		for (i = 0; i < 256; i++) {
			hyperfb_wait(sc);
			hyperfb_write4(sc, NGLE_REG_4,
			    (i << 16) | (i << 8) | i);
		}
		hyperfb_write4(sc, NGLE_REG_2, 0x0);
		hyperfb_write4(sc, NGLE_REG_38,
		    LBC_ENABLE | LBC_TYPE_CMAP | 0x100);
		hyperfb_wait(sc);
	}

	hyperfb_setup_fb(sc);

	/* make sure video output is enabled */
	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_33,
	    hyperfb_read4(sc, NGLE_REG_33) | 0x0a000000);

	/* cursor mask */
	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_30, 0);
	for (i = 0; i < 64; i++) {
		hyperfb_write4(sc, NGLE_REG_31, 0xffffffff);
		hyperfb_write4(sc, NGLE_REG_31, 0xffffffff);
	}

	/* cursor image */
	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_30, 0x80);
	for (i = 0; i < 64; i++) {
		hyperfb_write4(sc, NGLE_REG_31, 0xff00ff00);
		hyperfb_write4(sc, NGLE_REG_31, 0xff00ff00);
	}

	/* colour map */
	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_10, 0xBBE0F000);
	hyperfb_write4(sc, NGLE_REG_14, 0x03000300);
	hyperfb_write4(sc, NGLE_REG_13, 0xffffffff);
	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_3, 0);
	hyperfb_write4(sc, NGLE_REG_4, 0x000000ff);	/* BG */
	hyperfb_write4(sc, NGLE_REG_4, 0x00ff0000);	/* FG */
	hyperfb_wait(sc);
	hyperfb_write4(sc, NGLE_REG_2, 0);
	hyperfb_write4(sc, NGLE_REG_38, LBC_ENABLE | LBC_TYPE_CURSOR | 4);
	hyperfb_setup_fb(sc);

	hyperfb_move_cursor(sc, 100, 100);
}

static void
hyperfb_set_video(struct hyperfb_softc *sc, int on)
{
	uint32_t reg;

	if (sc->sc_video_on == on)
		return;

	sc->sc_video_on = on;

	hyperfb_wait(sc);
	reg = hyperfb_read4(sc, NGLE_REG_33);

	if (on) {
		hyperfb_write4(sc, NGLE_REG_33, reg | HCRX_VIDEO_ENABLE);
	} else {
		hyperfb_write4(sc, NGLE_REG_33, reg & ~HCRX_VIDEO_ENABLE);
	}
}

static void
hyperfb_rectfill(struct hyperfb_softc *sc, int x, int y, int wi, int he,
    uint32_t bg)
{
	uint32_t mask = 0xffffffff;

	/*
	 * XXX
	 * HCRX and EG both always draw rectangles at least 32 pixels wide
	 * for anything narrower we need to set a bit mask and enable 
	 * transparency
	 */

	if (sc->sc_hwmode != HW_FILL) {
		hyperfb_wait_fifo(sc, 3);
		/* plane mask */
		hyperfb_write4(sc, NGLE_REG_13, 0xff);
		/* bitmap op */
		hyperfb_write4(sc, NGLE_REG_14,
		    IBOvals(RopSrc, 0, BitmapExtent08, 0, DataDynamic, MaskOtc,
			1 /* bg transparent */, 0));
		/* dst bitmap access */
		hyperfb_write4(sc, NGLE_REG_11,
		    BA(IndexedDcd, Otc32, OtsIndirect, AddrLong, 0, BINovly,
			0));
		sc->sc_hwmode = HW_FILL;
	}
	hyperfb_wait_fifo(sc, 4);
	if (wi < 32)
		mask = 0xffffffff << (32 - wi);
	/*
	 * XXX - the NGLE code calls this 'transfer data'
	 * in reality it's a bit mask applied per pixel, 
	 * foreground colour in reg 35, bg in 36
	 */
	hyperfb_write4(sc, NGLE_REG_8, mask);

	hyperfb_write4(sc, NGLE_REG_35, bg);
	/* dst XY */
	hyperfb_write4(sc, NGLE_REG_6, (x << 16) | y);
	/* len XY start */
	hyperfb_write4(sc, NGLE_REG_9, (wi << 16) | he);
}

static void
hyperfb_bitblt(void *cookie, int xs, int ys, int xd, int yd, int wi,
    int he, int rop)
{
	struct hyperfb_softc *sc = cookie;

	if (sc->sc_hwmode != HW_BLIT) {
		hyperfb_wait(sc);
		hyperfb_write4(sc, NGLE_REG_10,
		    BA(IndexedDcd, Otc04, Ots08, AddrLong, 0, BINovly, 0));
		hyperfb_write4(sc, NGLE_REG_13, 0xff);
		sc->sc_hwmode = HW_BLIT;
	}
	hyperfb_wait_fifo(sc, 4);
	hyperfb_write4(sc, NGLE_REG_14, ((rop << 8) & 0xf00) | 0x23000000);
	/* IBOvals(rop, 0, BitmapExtent08, 1, DataDynamic, MaskOtc, 0, 0) */
	hyperfb_write4(sc, NGLE_REG_24, (xs << 16) | ys);
	hyperfb_write4(sc, NGLE_REG_7, (wi << 16) | he);
	hyperfb_write4(sc, NGLE_REG_25, (xd << 16) | yd);
}

static void
hyperfb_nuke_cursor(struct rasops_info *ri)
{
	struct vcons_screen *scr = ri->ri_hw;
	struct hyperfb_softc *sc = scr->scr_cookie;
	int wi, he, x, y;

	if (ri->ri_flg & RI_CURSOR) {
		wi = ri->ri_font->fontwidth;
		he = ri->ri_font->fontheight;
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		hyperfb_bitblt(sc, x, y, x, y, wi, he, RopInv);
		ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
hyperfb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct hyperfb_softc *sc = scr->scr_cookie;
	int x, y, wi, he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		if (on) {
			if (ri->ri_flg & RI_CURSOR) {
				hyperfb_nuke_cursor(ri);
			}
			x = col * wi + ri->ri_xorigin;
			y = row * he + ri->ri_yorigin;
			hyperfb_bitblt(sc, x, y, x, y, wi, he, RopInv);
			ri->ri_flg |= RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
	} else {
		ri->ri_crow = row;
		ri->ri_ccol = col;
		ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
hyperfb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct hyperfb_softc *sc = scr->scr_cookie;
	void *data;
	int i, x, y, wi, he/*, rv = GC_NOPE*/;
	uint32_t bg, fg, mask;

	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL)
		return;

	if (!CHAR_IN_FONT(c, font))
		return;

	if (row == ri->ri_crow && col == ri->ri_ccol) {
		ri->ri_flg &= ~RI_CURSOR;
	}

	wi = font->fontwidth;
	he = font->fontheight;

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	bg = ri->ri_devcmap[(attr >> 16) & 0xf];
	fg = ri->ri_devcmap[(attr >> 24) & 0x0f];

	/* clear the character cell */
	hyperfb_rectfill(sc, x, y, wi, he, bg);

	/* if we're drawing a space we're done here */
	if (c == 0x20) 
		return;

#if 0
	rv = glyphcache_try(&sc->sc_gc, c, x, y, attr);
	if (rv == GC_OK)
		return;
#endif

	data = WSFONT_GLYPH(c, font);

	hyperfb_wait_fifo(sc, 2);

	/* character colour */
	hyperfb_write4(sc, NGLE_REG_35, fg);
	/* dst XY */
	hyperfb_write4(sc, NGLE_REG_6, (x << 16) | y);

	/*
	 * drawing a rectangle moves the starting coordinates down the
	 * y-axis so we can just hammer the wi/he register to draw a full
	 * character
	 */
	if (ri->ri_font->stride == 1) {
		uint8_t *data8 = data;
		for (i = 0; i < he; i++) {
			hyperfb_wait_fifo(sc, 2);
			mask = *data8;
			hyperfb_write4(sc, NGLE_REG_8, mask << 24);	
			hyperfb_write4(sc, NGLE_REG_9, (wi << 16) | 1);
			data8++;
		}
	} else {
		uint16_t *data16 = data;
		for (i = 0; i < he; i++) {
			hyperfb_wait_fifo(sc, 2);
			mask = *data16;
			hyperfb_write4(sc, NGLE_REG_8, mask << 16);	
			hyperfb_write4(sc, NGLE_REG_9, (wi << 16) | 1);
			data16++;
		}
	}
#if 0
	if (rv == GC_ADD)
		glyphcache_add(&sc->sc_gc, c, x, y);
#endif
}

static void
hyperfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct hyperfb_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		if (ri->ri_crow == row &&
		   (ri->ri_ccol >= srccol && ri->ri_ccol < (srccol + ncols)) &&
		   (ri->ri_flg & RI_CURSOR)) {
			hyperfb_nuke_cursor(ri);
		}

		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		hyperfb_bitblt(sc, xs, y, xd, y, width, height, RopSrc);
		if (ri->ri_crow == row &&
		   (ri->ri_ccol >= dstcol && ri->ri_ccol < (dstcol + ncols)))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
hyperfb_erasecols(void *cookie, int row, int startcol, int ncols,
    long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct hyperfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		hyperfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
		if (ri->ri_crow == row &&
		    (ri->ri_ccol >= startcol &&
			ri->ri_ccol < (startcol + ncols)))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
hyperfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct hyperfb_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		if ((ri->ri_crow >= srcrow &&
			ri->ri_crow < (srcrow + nrows)) &&
		    (ri->ri_flg & RI_CURSOR)) {
			hyperfb_nuke_cursor(ri);
		}
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		hyperfb_bitblt(sc, x, ys, x, yd, width, height, RopSrc);
		if (ri->ri_crow >= dstrow && ri->ri_crow < (dstrow + nrows))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
hyperfb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct hyperfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		hyperfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);

		if (ri->ri_crow >= row && ri->ri_crow < (row + nrows))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
hyperfb_move_cursor(struct hyperfb_softc *sc, int x, int y)
{
	uint32_t pos;

	sc->sc_cursor_x = x;
	x -= sc->sc_hot_x;
	sc->sc_cursor_y = y;
	y -= sc->sc_hot_y;

	if (x < 0) x = 0x1000 - x;
	if (y < 0) y = 0x1000 - y;
	pos = (x << 16) | y;
	if (sc->sc_enabled) pos |= HCRX_ENABLE_CURSOR;
	hyperfb_wait_fifo(sc, 2);
	hyperfb_write4(sc, NGLE_REG_28, 0);
	hyperfb_write4(sc, NGLE_REG_29, pos);
}

static int
hyperfb_do_cursor(struct hyperfb_softc *sc, struct wsdisplay_cursor *cur)
{

	if (cur->which & WSDISPLAY_CURSOR_DOCUR) {

		sc->sc_enabled = cur->enable;
		cur->which |= WSDISPLAY_CURSOR_DOPOS;
	}
	if (cur->which & WSDISPLAY_CURSOR_DOHOT) {

		sc->sc_hot_x = cur->hot.x;
		sc->sc_hot_y = cur->hot.y;
		cur->which |= WSDISPLAY_CURSOR_DOPOS;
	}
	if (cur->which & WSDISPLAY_CURSOR_DOPOS) {

		hyperfb_move_cursor(sc, cur->pos.x, cur->pos.y);
	}
	if (cur->which & WSDISPLAY_CURSOR_DOCMAP) {
		uint32_t rgb;
		uint8_t r[2], g[2], b[2];

		copyin(cur->cmap.blue, b, 2);
		copyin(cur->cmap.green, g, 2);
		copyin(cur->cmap.red, r, 2);
		mutex_enter(&sc->sc_hwlock);
		hyperfb_wait(sc);
		hyperfb_write4(sc, NGLE_REG_10, 0xBBE0F000);
		hyperfb_write4(sc, NGLE_REG_14, 0x03000300);
		hyperfb_write4(sc, NGLE_REG_13, 0xffffffff);
		hyperfb_wait(sc);
		hyperfb_write4(sc, NGLE_REG_3, 0);
		rgb = (r[0] << 16) | (g[0] << 8) | b[0];
		hyperfb_write4(sc, NGLE_REG_4, rgb);	/* BG */
		rgb = (r[1] << 16) | (g[1] << 8) | b[1];
		hyperfb_write4(sc, NGLE_REG_4, rgb);	/* FG */
		hyperfb_write4(sc, NGLE_REG_2, 0);
		hyperfb_write4(sc, NGLE_REG_38,
		    LBC_ENABLE | LBC_TYPE_CURSOR | 4);

		hyperfb_setup_fb(sc);
		mutex_exit(&sc->sc_hwlock);

	}
	if (cur->which & WSDISPLAY_CURSOR_DOSHAPE) {
		uint32_t buffer[128], latch, tmp;
		int i;

		copyin(cur->mask, buffer, 512);
		hyperfb_wait(sc);
		hyperfb_write4(sc, NGLE_REG_30, 0);
		for (i = 0; i < 128; i += 2) {
			latch = 0;
			tmp = buffer[i] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i] & 0x01010101;
			latch |= tmp << 7;
			hyperfb_write4(sc, NGLE_REG_31, latch);
			latch = 0;
			tmp = buffer[i + 1] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i + 1] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i + 1] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i + 1] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i + 1] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i + 1] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i + 1] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i + 1] & 0x01010101;
			latch |= tmp << 7;
			hyperfb_write4(sc, NGLE_REG_31, latch);
		}

		copyin(cur->image, buffer, 512);
		hyperfb_wait(sc);
		hyperfb_write4(sc, NGLE_REG_30, 0x80);
		for (i = 0; i < 128; i += 2) {
			latch = 0;
			tmp = buffer[i] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i] & 0x01010101;
			latch |= tmp << 7;
			hyperfb_write4(sc, NGLE_REG_31, latch);
			latch = 0;
			tmp = buffer[i + 1] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i + 1] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i + 1] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i + 1] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i + 1] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i + 1] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i + 1] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i + 1] & 0x01010101;
			latch |= tmp << 7;
			hyperfb_write4(sc, NGLE_REG_31, latch);
		}
		hyperfb_setup_fb(sc);
	}

	return 0;
}
