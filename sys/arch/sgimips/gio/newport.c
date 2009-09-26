/*	$NetBSD: newport.c,v 1.10.54.3 2009/09/26 17:51:56 snj Exp $	*/

/*
 * Copyright (c) 2003 Ilpo Ruotsalainen
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
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: newport.c,v 1.10.54.3 2009/09/26 17:51:56 snj Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/sysconf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <sgimips/gio/giovar.h>
#include <sgimips/gio/newportvar.h>
#include <sgimips/gio/newportreg.h>

struct newport_softc {
	device_t sc_dev;

	struct newport_devconfig *sc_dc;
	
};

struct newport_devconfig {
	uint32_t		dc_addr;

	bus_space_tag_t		dc_st;
	bus_space_handle_t	dc_sh;

	int			dc_boardrev;
	int			dc_vc2rev;
	int			dc_cmaprev;
	int			dc_xmaprev;
	int			dc_rexrev;
	int			dc_xres;
	int			dc_yres;
	int			dc_depth;

	int			dc_font;
	struct wsscreen_descr	*dc_screen;
	struct wsdisplay_font	*dc_fontdata;
	struct vcons_data	dc_vd;
};

static int  newport_match(device_t, struct cfdata *, void *);
static void newport_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(newport, sizeof(struct newport_softc),
    newport_match, newport_attach, NULL, NULL);

/* textops */
static void newport_cursor(void *, int, int, int);
static void newport_cursor_dummy(void *, int, int, int);
static int  newport_mapchar(void *, int, unsigned int *);
static void newport_putchar(void *, int, int, u_int, long);
static void newport_copycols(void *, int, int, int, int);
static void newport_erasecols(void *, int, int, int, long);
static void newport_copyrows(void *, int, int, int);
static void newport_eraserows(void *, int, int, long);
static int  newport_allocattr(void *, int, int, int, long *);

static void newport_init_screen(void *, struct vcons_screen *, int, long *);

/* accessops */
static int     newport_ioctl(void *, void *, u_long, void *, int,
    struct lwp *);
static paddr_t newport_mmap(void *, void *, off_t, int);

static struct wsdisplay_accessops newport_accessops = {
	.ioctl		= newport_ioctl,
	.mmap		= newport_mmap,
};

static struct wsdisplay_emulops newport_textops = {
	.cursor		= newport_cursor_dummy,
	.mapchar	= newport_mapchar,
	.putchar	= newport_putchar,
	.copycols	= newport_copycols,
	.erasecols	= newport_erasecols,
	.copyrows	= newport_copyrows,
	.eraserows	= newport_eraserows,
	.allocattr	= newport_allocattr
};

static struct wsscreen_descr newport_screen_1024x768 = {
	.name		= "1024x768",
	.ncols		= 128,
	.nrows		= 48,
	.textops	= &newport_textops,
	.fontwidth	= 8,
	.fontheight	= 16,
	.capabilities	= WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_REVERSE
};

static struct wsscreen_descr newport_screen_1280x1024 = {
	.name		= "1280x1024",
	.ncols		= 160,
	.nrows		= 64,
	.textops	= &newport_textops,
	.fontwidth	= 8,
	.fontheight	= 16,
	.capabilities	= WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_REVERSE
};

static const struct wsscreen_descr *_newport_screenlist[] = {
	&newport_screen_1024x768,
	&newport_screen_1280x1024
};

static const struct wsscreen_list newport_screenlist = {
	sizeof(_newport_screenlist) / sizeof(struct wsscreen_descr *),
	_newport_screenlist
};

static struct vcons_screen newport_console_screen;
static struct newport_devconfig newport_console_dc;
static int newport_is_console = 0;

#define NEWPORT_ATTR_ENCODE(fg,bg)	(((fg) << 8) | (bg))
#define NEWPORT_ATTR_BG(a)		((a) & 0xff)
#define NEWPORT_ATTR_FG(a)		(((a) >> 8) & 0xff)

extern const u_char rasops_cmap[768];

/**** Low-level hardware register groveling functions ****/
static void
rex3_write(struct newport_devconfig *dc, bus_size_t rexreg, uint32_t val)
{
	bus_space_write_4(dc->dc_st, dc->dc_sh, NEWPORT_REX3_OFFSET + rexreg,
	    val);
}

static void
rex3_write_go(struct newport_devconfig *dc, bus_size_t rexreg, uint32_t val)
{
	rex3_write(dc, rexreg + REX3_REG_GO, val);
}

static uint32_t
rex3_read(struct newport_devconfig *dc, bus_size_t rexreg)
{
	return bus_space_read_4(dc->dc_st, dc->dc_sh, NEWPORT_REX3_OFFSET +
	    rexreg);
}

static void
rex3_wait_gfifo(struct newport_devconfig *dc)
{
	while (rex3_read(dc, REX3_REG_STATUS) & REX3_STATUS_GFXBUSY)
		;
}

static void
vc2_write_ireg(struct newport_devconfig *dc, uint8_t ireg, uint16_t val)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_3 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_INDEX << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, (ireg << 24) | (val << 8));
}

static uint16_t
vc2_read_ireg(struct newport_devconfig *dc, uint8_t ireg)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_INDEX << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, ireg << 24);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_IREG << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	return (uint16_t)(rex3_read(dc, REX3_REG_DCBDATA0) >> 16);
}

static uint16_t
vc2_read_ram(struct newport_devconfig *dc, uint16_t addr)
{
	vc2_write_ireg(dc, VC2_IREG_RAM_ADDRESS, addr);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_RAM << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	return (uint16_t)(rex3_read(dc, REX3_REG_DCBDATA0) >> 16);
}

#if 0
static void
vc2_write_ram(struct newport_devconfig *dc, uint16_t addr, uint16_t val)
{
	vc2_write_ireg(dc, VC2_IREG_RAM_ADDRESS, addr);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_RAM << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, val << 16);
}
#endif

static u_int32_t
xmap9_read(struct newport_devconfig *dc, int crs)
{
	rex3_write(dc, REX3_REG_DCBMODE,
		REX3_DCBMODE_DW_1 |
		(NEWPORT_DCBADDR_XMAP_0 << REX3_DCBMODE_DCBADDR_SHIFT) |
		(crs << REX3_DCBMODE_DCBCRS_SHIFT) |
		(3 << REX3_DCBMODE_CSWIDTH_SHIFT) |
		(2 << REX3_DCBMODE_CSHOLD_SHIFT) |
		(1 << REX3_DCBMODE_CSSETUP_SHIFT));
	return rex3_read(dc, REX3_REG_DCBDATA0);
}

static void
xmap9_write(struct newport_devconfig *dc, int crs, uint8_t val)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    (NEWPORT_DCBADDR_XMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (crs << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (3 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (2 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, val << 24);
}

static void
xmap9_write_mode(struct newport_devconfig *dc, uint8_t index, uint32_t mode)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_4 |
	    (NEWPORT_DCBADDR_XMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (XMAP9_DCBCRS_MODE_SETUP << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (3 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (2 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, (index << 24) | mode);
}

/**** Helper functions ****/
static void
newport_fill_rectangle(struct newport_devconfig *dc, int x1, int y1, int x2,
    int y2, uint8_t color)
{
	rex3_wait_gfifo(dc);
	
	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_DRAW |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_DOSETUP |
	    REX3_DRAWMODE0_STOPONX | REX3_DRAWMODE0_STOPONY);
	rex3_write(dc, REX3_REG_DRAWMODE1,
	    REX3_DRAWMODE1_PLANES_CI |
	    REX3_DRAWMODE1_DD_DD8 |
	    REX3_DRAWMODE1_RWPACKED |
	    REX3_DRAWMODE1_HD_HD8 |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    REX3_DRAWMODE1_LO_SRC);
	rex3_write(dc, REX3_REG_WRMASK, 0xffffffff);
	rex3_write(dc, REX3_REG_COLORI, color);
	rex3_write(dc, REX3_REG_XYSTARTI, (x1 << REX3_XYSTARTI_XSHIFT) | y1);

	rex3_write_go(dc, REX3_REG_XYENDI, (x2 << REX3_XYENDI_XSHIFT) | y2);
}

static void
newport_bitblt(struct newport_devconfig *dc, int xs, int ys, int xd,
    int yd, int wi, int he, int rop)
{
	int xe, ye;
	uint32_t tmp;

	rex3_wait_gfifo(dc);
	if (yd > ys) {
		/* need to copy bottom up */
		ye = ys;
		yd += he - 1;
		ys += he - 1;
	} else
		ye = ys + he - 1;

	if (xd > xs) {
		/* need to copy right to left */
		xe = xs;
		xd += wi - 1;
		xs += wi - 1;
	} else
		xe = xs + wi - 1;

	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_SCR2SCR |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_DOSETUP |
	    REX3_DRAWMODE0_STOPONX | REX3_DRAWMODE0_STOPONY);
	rex3_write(dc, REX3_REG_DRAWMODE1,
	    REX3_DRAWMODE1_PLANES_CI |
	    REX3_DRAWMODE1_DD_DD8 |
	    REX3_DRAWMODE1_RWPACKED |
	    REX3_DRAWMODE1_HD_HD8 |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    ((rop << 28) & REX3_DRAWMODE1_LOGICOP_MASK));
	rex3_write(dc, REX3_REG_XYSTARTI, (xs << REX3_XYSTARTI_XSHIFT) | ys);
	rex3_write(dc, REX3_REG_XYENDI, (xe << REX3_XYENDI_XSHIFT) | ye);

	tmp = (yd - ys) & 0xffff;
	tmp |= (xd - xs) << REX3_XYMOVE_XSHIFT;

	rex3_write_go(dc, REX3_REG_XYMOVE, tmp);
}

static void
newport_cmap_setrgb(struct newport_devconfig *dc, int index, uint8_t r,
    uint8_t g, uint8_t b)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_CMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (CMAP_DCBCRS_ADDRESS_LOW << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT) |
	    REX3_DCBMODE_SWAPENDIAN);

	rex3_write(dc, REX3_REG_DCBDATA0, index << 16);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_3 |
	    (NEWPORT_DCBADDR_CMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (CMAP_DCBCRS_PALETTE << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, (r << 24) + (g << 16) + (b << 8));
}

static void
newport_get_resolution(struct newport_devconfig *dc)
{
	uint16_t vep,lines;
	uint16_t linep,cols;
	uint16_t data;

	vep = vc2_read_ireg(dc, VC2_IREG_VIDEO_ENTRY);

	dc->dc_xres = 0;
	dc->dc_yres = 0;

	for (;;) {
		/* Iterate over runs in video timing table */

		cols = 0;

		linep = vc2_read_ram(dc, vep++);
		lines = vc2_read_ram(dc, vep++);

		if (lines == 0)
			break;

		do {
			/* Iterate over state runs in line sequence table */
		
			data = vc2_read_ram(dc, linep++);

			if ((data & 0x0001) == 0)
				cols += (data >> 7) & 0xfe;

			if ((data & 0x0080) == 0)
				data = vc2_read_ram(dc, linep++);
		} while ((data & 0x8000) == 0);

		if (cols != 0) {
			if (cols > dc->dc_xres)
				dc->dc_xres = cols;

			dc->dc_yres += lines;
		}
	}
}

static void
newport_setup_hw(struct newport_devconfig *dc)
{
	uint16_t curp,tmp;
	int i;
	uint32_t scratch;

	/* Get various revisions */
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    (NEWPORT_DCBADDR_CMAP_0 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (CMAP_DCBCRS_REVISION << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	scratch = vc2_read_ireg(dc, VC2_IREG_CONFIG);
	dc->dc_vc2rev = (scratch & VC2_IREG_CONFIG_REVISION) >> 5;

	scratch = rex3_read(dc, REX3_REG_DCBDATA0);

	dc->dc_boardrev = (scratch >> 28) & 0x07;
	dc->dc_cmaprev = scratch & 0x07;
	dc->dc_xmaprev = xmap9_read(dc, XMAP9_DCBCRS_REVISION) & 0x07;
	dc->dc_depth = ( (dc->dc_boardrev > 1) && (scratch & 0x80)) ? 8 : 24;

	/* Setup cursor glyph */
	curp = vc2_read_ireg(dc, VC2_IREG_CURSOR_ENTRY);

	/* Setup VC2 to a known state */
	tmp = vc2_read_ireg(dc, VC2_IREG_CONTROL) & VC2_CONTROL_INTERLACE;
	vc2_write_ireg(dc, VC2_IREG_CONTROL, tmp |
	    VC2_CONTROL_DISPLAY_ENABLE |
	    VC2_CONTROL_VTIMING_ENABLE |
	    VC2_CONTROL_DID_ENABLE |
	    VC2_CONTROL_CURSORFUNC_ENABLE /*|
	    VC2_CONTROL_CURSOR_ENABLE*/);

	/* Setup XMAP9s */
	xmap9_write(dc, XMAP9_DCBCRS_CONFIG,
	    XMAP9_CONFIG_8BIT_SYSTEM | XMAP9_CONFIG_RGBMAP_CI);

	xmap9_write(dc, XMAP9_DCBCRS_CURSOR_CMAP, 0);

	xmap9_write_mode(dc, 0,
	    XMAP9_MODE_GAMMA_BYPASS |
	    XMAP9_MODE_PIXSIZE_8BPP);
	xmap9_write(dc, XMAP9_DCBCRS_MODE_SELECT, 0);

	/* Setup REX3 */
	rex3_write(dc, REX3_REG_XYWIN, (4096 << 16) | 4096);
	rex3_write(dc, REX3_REG_TOPSCAN, 0x3ff); /* XXX Why? XXX */

	/* Setup CMAP */
	for (i = 0; i < 256; i++)
		newport_cmap_setrgb(dc, i, rasops_cmap[i * 3],
		    rasops_cmap[i * 3 + 1], rasops_cmap[i * 3 + 2]);
}

/**** Attach routines ****/
static int
newport_match(device_t parent, struct cfdata *self, void *aux)
{
	struct gio_attach_args *ga = aux;

	/* newport doesn't decode all addresses */
	if (ga->ga_addr != 0x1f000000 && ga->ga_addr != 0x1f400000 &&
	    ga->ga_addr != 0x1f800000 && ga->ga_addr != 0x1fc00000)
		return 0;

	/* Don't do the destructive probe if we're already attached */
	if (newport_is_console && ga->ga_addr == newport_console_dc.dc_addr)
		return 1;

	if (platform.badaddr(
	    (void *)(ga->ga_ioh + NEWPORT_REX3_OFFSET + REX3_REG_XSTARTI),
	    sizeof(uint32_t)))
		return 0;
	if (platform.badaddr(
	    (void *)(ga->ga_ioh + NEWPORT_REX3_OFFSET + REX3_REG_XSTART),
	    sizeof(uint32_t)))
		return 0;

	/* Ugly, this probe is destructive, blame SGI... */
	/* XXX Should be bus_space_peek/bus_space_poke XXX */
	bus_space_write_4(ga->ga_iot, ga->ga_ioh,
	    NEWPORT_REX3_OFFSET + REX3_REG_XSTARTI, 0x12345678);
	if (bus_space_read_4(ga->ga_iot, ga->ga_ioh,
	      NEWPORT_REX3_OFFSET + REX3_REG_XSTART)
	    != ((0x12345678 & 0xffff) << 11))
		return 0;
	
	return 1;
}

static void
newport_attach_common(struct newport_devconfig *dc, struct gio_attach_args *ga)
{
	dc->dc_addr = ga->ga_addr;

	dc->dc_st = ga->ga_iot;
	dc->dc_sh = ga->ga_ioh;

	wsfont_init();

	dc->dc_font = wsfont_find(NULL, 8, 16, 0, WSDISPLAY_FONTORDER_L2R,
	    WSDISPLAY_FONTORDER_L2R);
	if (dc->dc_font < 0)
		panic("newport_attach_common: no suitable fonts");

	if (wsfont_lock(dc->dc_font, &dc->dc_fontdata))
		panic("newport_attach_common: unable to lock font data");

	newport_setup_hw(dc);

	newport_get_resolution(dc);

	newport_fill_rectangle(dc, 0, 0, dc->dc_xres, dc->dc_yres, 0);
	if (dc->dc_xres == 1280) {
		dc->dc_screen = &newport_screen_1280x1024;
	} else
		dc->dc_screen = &newport_screen_1024x768;
	
}

static void
newport_attach(device_t parent, device_t self, void *aux)
{
	struct gio_attach_args *ga = aux;
	struct newport_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args wa;
	unsigned long defattr;

	sc->sc_dev = self;
	if (newport_is_console && ga->ga_addr == newport_console_dc.dc_addr) {
		wa.console = 1;
		sc->sc_dc = &newport_console_dc;
	} else {
		wa.console = 0;
		sc->sc_dc = malloc(sizeof(struct newport_devconfig),
		    M_DEVBUF, M_WAITOK | M_ZERO);
		if (sc->sc_dc == NULL)
			panic("newport_attach: out of memory");

		newport_attach_common(sc->sc_dc, ga);
	}

	aprint_naive(": Display adapter\n");

	aprint_normal(": SGI NG1 (board revision %d, cmap revision %d, xmap revision %d, vc2 revision %d), depth %d\n",
	    sc->sc_dc->dc_boardrev, sc->sc_dc->dc_cmaprev,
	    sc->sc_dc->dc_xmaprev, sc->sc_dc->dc_vc2rev, sc->sc_dc->dc_depth);
	vcons_init(&sc->sc_dc->dc_vd, sc->sc_dc, sc->sc_dc->dc_screen,
	    &newport_accessops);
	sc->sc_dc->dc_vd.init_screen = newport_init_screen;
	if (newport_is_console) {
		newport_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
		vcons_init_screen(&sc->sc_dc->dc_vd, &newport_console_screen,
		    1, &defattr);
		sc->sc_dc->dc_screen->textops =
		    &newport_console_screen.scr_ri.ri_ops;
		memcpy(&newport_textops, &newport_console_screen.scr_ri.ri_ops,
		    sizeof(struct wsdisplay_emulops));
	}
	wa.scrdata = &newport_screenlist;
	wa.accessops = &newport_accessops;
	wa.accesscookie = &sc->sc_dc->dc_vd;

	config_found(sc->sc_dev, &wa, wsemuldisplaydevprint);
}

int
newport_cnattach(struct gio_attach_args *ga)
{
	struct rasops_info *ri = &newport_console_screen.scr_ri;
	long defattr = NEWPORT_ATTR_ENCODE(WSCOL_WHITE, WSCOL_BLACK);
	const struct wsscreen_descr *screen;

	if (!newport_match(NULL, NULL, ga)) {
		return ENXIO;
	}

	newport_attach_common(&newport_console_dc, ga);

	if (newport_console_dc.dc_xres >= 1280 &&
	    newport_console_dc.dc_yres >= 1024)
		screen = &newport_screen_1280x1024;
	else
		screen = &newport_screen_1024x768;

	ri->ri_hw = &newport_console_screen;
	ri->ri_depth = newport_console_dc.dc_depth;
	ri->ri_width = newport_console_dc.dc_xres;
	ri->ri_height = newport_console_dc.dc_yres;
	ri->ri_stride = newport_console_dc.dc_xres; /* XXX */
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;
	ri->ri_ops.copyrows  = newport_copyrows;
	ri->ri_ops.eraserows = newport_eraserows;
	ri->ri_ops.copycols  = newport_copycols;
	ri->ri_ops.erasecols = newport_erasecols;
	ri->ri_ops.cursor    = newport_cursor_dummy;
	ri->ri_ops.mapchar   = newport_mapchar;
	ri->ri_ops.putchar   = newport_putchar;
	ri->ri_ops.allocattr = newport_allocattr;
	ri->ri_font = newport_console_dc.dc_fontdata;
	newport_console_screen.scr_cookie = &newport_console_dc;

	newport_putchar(ri, 3, 3, 0x41, defattr);
	wsdisplay_cnattach(screen, ri, 0, 0, defattr);
	newport_putchar(ri, 4, 3, 0x42, defattr);

	newport_is_console = 1;

	return 0;
}

static void
newport_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct newport_devconfig *dc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = dc->dc_depth;
	ri->ri_width = dc->dc_xres;
	ri->ri_height = dc->dc_yres;
	ri->ri_stride = dc->dc_xres; /* XXX */
	ri->ri_flg = RI_CENTER;

	/*&ri->ri_bits = (char *)sc->sc_fb.fb_pixels;*/

	rasops_init(ri, dc->dc_yres / 8, dc->dc_xres / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, dc->dc_yres / ri->ri_font->fontheight,
		    dc->dc_xres / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	ri->ri_ops.copyrows  = newport_copyrows;
	ri->ri_ops.eraserows = newport_eraserows;
	ri->ri_ops.copycols  = newport_copycols;
	ri->ri_ops.erasecols = newport_erasecols;
	ri->ri_ops.cursor    = newport_cursor;
	ri->ri_ops.mapchar   = newport_mapchar;
	ri->ri_ops.putchar   = newport_putchar;
	ri->ri_ops.allocattr = newport_allocattr;
}

/**** wsdisplay textops ****/
static void
newport_cursor_dummy(void *c, int on, int row, int col)
{
}

static void
newport_cursor(void *c, int on, int row, int col)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	int x, y, wi,he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if (ri->ri_flg & RI_CURSOR) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		newport_bitblt(dc, x, y, x, y, wi, he, 12);
		ri->ri_flg &= ~RI_CURSOR;
	}

	ri->ri_crow = row;
	ri->ri_ccol = col;

	if (on)
	{
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		newport_bitblt(dc, x, y, x, y, wi, he, 12);
		ri->ri_flg |= RI_CURSOR;
	}
}

static int
newport_mapchar(void *c, int ch, unsigned int *cp)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;

	if (dc->dc_fontdata->encoding != WSDISPLAY_FONTENC_ISO) {
		ch = wsfont_map_unichar(dc->dc_fontdata, ch);

		if (ch < 0)
			goto fail;
	}

	if (ch < dc->dc_fontdata->firstchar ||
	    ch >= dc->dc_fontdata->firstchar + dc->dc_fontdata->numchars)
		goto fail;

	*cp = ch;
	return 5;

fail:
	*cp = ' ';
	return 0;
}

static void
newport_putchar(void *c, int row, int col, u_int ch, long attr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	struct wsdisplay_font *font = ri->ri_font;
	uint8_t *bitmap = (u_int8_t *)font->data + (ch - font->firstchar) * 
	    font->fontheight * font->stride;
	uint32_t pattern;
	int i;
	int x = col * font->fontwidth + ri->ri_xorigin;
	int y = row * font->fontheight + ri->ri_yorigin;

	rex3_wait_gfifo(dc);
	
	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_DRAW |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_STOPONX |
	    REX3_DRAWMODE0_ENZPATTERN | REX3_DRAWMODE0_ZPOPAQUE);

	rex3_write(dc, REX3_REG_DRAWMODE1,
	    REX3_DRAWMODE1_PLANES_CI |
	    REX3_DRAWMODE1_DD_DD8 |
	    REX3_DRAWMODE1_RWPACKED |
	    REX3_DRAWMODE1_HD_HD8 |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    REX3_DRAWMODE1_LO_SRC);

	rex3_write(dc, REX3_REG_XYSTARTI, (x << REX3_XYSTARTI_XSHIFT) | y);
	rex3_write(dc, REX3_REG_XYENDI,
	    (x + font->fontwidth - 1) << REX3_XYENDI_XSHIFT);

	rex3_write(dc, REX3_REG_COLORI, NEWPORT_ATTR_FG(attr));
	rex3_write(dc, REX3_REG_COLORBACK, NEWPORT_ATTR_BG(attr));

	rex3_write(dc, REX3_REG_WRMASK, 0xffffffff);

	for (i = 0; i < font->fontheight; i++) {
		/* XXX Works only with font->fontwidth == 8 XXX */
		pattern = *bitmap << 24;
		
		rex3_write_go(dc, REX3_REG_ZPATTERN, pattern);

		bitmap += font->stride;
	}
}

static void
newport_copycols(void *c, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;

	xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
	xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
	y = ri->ri_yorigin + ri->ri_font->fontheight * row;
	width = ri->ri_font->fontwidth * ncols;
	height = ri->ri_font->fontheight;
	newport_bitblt(dc, xs, y, xd, y, width, height, 3);
}

static void
newport_erasecols(void *c, int row, int startcol, int ncols,
    long attr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	struct wsdisplay_font *font = dc->dc_fontdata;

	newport_fill_rectangle(dc,
	    startcol * font->fontwidth,				/* x1 */
	    row * font->fontheight,				/* y1 */
	    (startcol + ncols) * font->fontwidth - 1,		/* x2 */
	    (row + 1) * font->fontheight - 1,			/* y2 */
	    NEWPORT_ATTR_BG(attr));
}

static void
newport_copyrows(void *c, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	x = ri->ri_xorigin;
	ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
	yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
	width = ri->ri_emuwidth;
	height = ri->ri_font->fontheight * nrows;

	newport_bitblt(dc, x, ys, x, yd, width, height, 3);
}

static void
newport_eraserows(void *c, int startrow, int nrows, long attr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	struct wsdisplay_font *font = dc->dc_fontdata;

	newport_fill_rectangle(dc,
	    0,							/* x1 */
	    startrow * font->fontheight,			/* y1 */
	    dc->dc_xres,					/* x2 */
	    (startrow + nrows) * font->fontheight - 1,		/* y2 */
	    NEWPORT_ATTR_BG(attr));
}

static int
newport_allocattr(void *c, int fg, int bg, int flags, long *attr)
{
	if (flags & WSATTR_BLINK)
		return EINVAL;

	if ((flags & WSATTR_WSCOLORS) == 0) {
		fg = WSCOL_WHITE;
		bg = WSCOL_BLACK;
	}

	if (flags & WSATTR_HILIT)
		fg += 8;

	if (flags & WSATTR_REVERSE) {
		int tmp = fg;
		fg = bg;
		bg = tmp;
	}

	*attr = NEWPORT_ATTR_ENCODE(fg, bg);

	return 0;
}

/**** wsdisplay accessops ****/

static int
newport_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd;
	struct newport_devconfig *dc;
	struct vcons_screen *ms;
#if 0
	struct wsdisplay_fbinfo *wdf;
	int nmode;
#endif
	vd = (struct vcons_data *)v;
	dc = (struct newport_devconfig *)vd->cookie;
	ms = (struct vcons_screen *)vd->active;

#define FBINFO (*(struct wsdisplay_fbinfo*)data)

	switch (cmd) {
	case WSDISPLAYIO_GINFO:
		FBINFO.width  = dc->dc_xres;
		FBINFO.height = dc->dc_yres;
		FBINFO.depth  = dc->dc_depth;
		FBINFO.cmsize = 1 << FBINFO.depth;
		return 0;
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_NEWPORT;
		return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
newport_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd;
	struct newport_devconfig *dc;

	vd = (struct vcons_data *)v;
	dc = (struct newport_devconfig *)vd->cookie;

	if ( offset >= 0xfffff)
		return -1;

	return mips_btop(dc->dc_addr + offset);
}
