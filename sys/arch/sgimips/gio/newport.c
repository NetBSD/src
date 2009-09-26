/*	$NetBSD: newport.c,v 1.10.54.1 2009/09/26 17:36:18 snj Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: newport.c,v 1.10.54.1 2009/09/26 17:36:18 snj Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/sysconf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wsfont/wsfont.h>

#include <sgimips/gio/giovar.h>
#include <sgimips/gio/newportvar.h>
#include <sgimips/gio/newportreg.h>

struct newport_softc {
	struct device sc_dev;

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
	struct wsdisplay_font	*dc_fontdata;
};

static int  newport_match(struct device *, struct cfdata *, void *);
static void newport_attach(struct device *, struct device *, void *);

CFATTACH_DECL(newport, sizeof(struct newport_softc),
    newport_match, newport_attach, NULL, NULL);

/* textops */
static void newport_cursor(void *, int, int, int);
static int  newport_mapchar(void *, int, unsigned int *);
static void newport_putchar(void *, int, int, u_int, long);
static void newport_copycols(void *, int, int, int, int);
static void newport_erasecols(void *, int, int, int, long);
static void newport_copyrows(void *, int, int, int);
static void newport_eraserows(void *, int, int, long);
static int  newport_allocattr(void *, int, int, int, long *);

/* accessops */
static int     newport_ioctl(void *, void *, u_long, void *, int,
    struct lwp *);
static paddr_t newport_mmap(void *, void *, off_t, int);
static int     newport_alloc_screen(void *, const struct wsscreen_descr *,
    void **, int *, int *, long *);
static void    newport_free_screen(void *, void *);
static int     newport_show_screen(void *, void *, int,
    void (*)(void *, int, int), void *);

static const struct wsdisplay_emulops newport_textops = {
	.cursor		= newport_cursor,
	.mapchar	= newport_mapchar,
	.putchar	= newport_putchar,
	.copycols	= newport_copycols,
	.erasecols	= newport_erasecols,
	.copyrows	= newport_copyrows,
	.eraserows	= newport_eraserows,
	.allocattr	= newport_allocattr
};

static const struct wsdisplay_accessops newport_accessops = {
	.ioctl		= newport_ioctl,
	.mmap		= newport_mmap,
	.alloc_screen	= newport_alloc_screen,
	.free_screen	= newport_free_screen,
	.show_screen	= newport_show_screen,
};

static const struct wsscreen_descr newport_screen_1024x768 = {
	.name		= "1024x768",
	.ncols		= 128,
	.nrows		= 48,
	.textops	= &newport_textops,
	.fontwidth	= 8,
	.fontheight	= 16,
	.capabilities	= WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_REVERSE
};

static const struct wsscreen_descr newport_screen_1280x1024 = {
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

static struct newport_devconfig newport_console_dc;
static int newport_is_console = 0;

#define NEWPORT_ATTR_ENCODE(fg,bg)	(((fg) << 8) | (bg))
#define NEWPORT_ATTR_BG(a)		((a) & 0xff)
#define NEWPORT_ATTR_FG(a)		(((a) >> 8) & 0xff)

static const uint16_t newport_cursor_data[128] = {
	/* Bit 0 */
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0xff00, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,

	/* Bit 1 */
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
};

static const uint8_t newport_defcmap[16*3] = {
	/* Normal colors */
	0x00, 0x00, 0x00, /* black */
	0x7f, 0x00, 0x00, /* red */
	0x00, 0x7f, 0x00, /* green */
	0x7f, 0x7f, 0x00, /* brown */
	0x00, 0x00, 0x7f, /* blue */
	0x7f, 0x00, 0x7f, /* magenta */
	0x00, 0x7f, 0x7f, /* cyan */
	0xc7, 0xc7, 0xc7, /* white - XXX too dim? */

	/* Hilite colors */
	0x7f, 0x7f, 0x7f, /* black */
	0xff, 0x00, 0x00, /* red */
	0x00, 0xff, 0x00, /* green */
	0xff, 0xff, 0x00, /* brown */
	0x00, 0x00, 0xff, /* blue */
	0xff, 0x00, 0xff, /* magenta */
	0x00, 0xff, 0xff, /* cyan */
	0xff, 0xff, 0xff, /* white */
};

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
	rex3_write(dc, REX3_REG_WRMASK, 0xffffffff);
	rex3_write(dc, REX3_REG_COLORI, color);
	rex3_write(dc, REX3_REG_XYSTARTI, (x1 << REX3_XYSTARTI_XSHIFT) | y1);

	rex3_write_go(dc, REX3_REG_XYENDI, (x2 << REX3_XYENDI_XSHIFT) | y2);
}

static void
newport_copy_rectangle(struct newport_devconfig *dc, int x1, int y1, int x2,
    int y2, int dx, int dy)
{
	uint32_t tmp;

	rex3_wait_gfifo(dc);
	if (dy > y1) {
		/* need to copy bottom up */
		dy += (y2 - y1);
		tmp = y2;
		y2 = y1;
		y1 = tmp;
	}

	if (dx > x1) {
		/* need to copy right to left */
		dx += (x2 - x1);
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}

	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_SCR2SCR |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_DOSETUP |
	    REX3_DRAWMODE0_STOPONX | REX3_DRAWMODE0_STOPONY);
	rex3_write(dc, REX3_REG_XYSTARTI, (x1 << REX3_XYSTARTI_XSHIFT) | y1);
	rex3_write(dc, REX3_REG_XYENDI, (x2 << REX3_XYENDI_XSHIFT) | y2);

	tmp = (dy - y1) & 0xffff;
	tmp |= (dx - x1) << REX3_XYMOVE_XSHIFT;

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

	for (i=0; i<128; i++)
		vc2_write_ram(dc, curp + i, newport_cursor_data[i]);

	/* Setup VC2 to a known state */
	tmp = vc2_read_ireg(dc, VC2_IREG_CONTROL) & VC2_CONTROL_INTERLACE;
	vc2_write_ireg(dc, VC2_IREG_CONTROL, tmp |
	    VC2_CONTROL_DISPLAY_ENABLE |
	    VC2_CONTROL_VTIMING_ENABLE |
	    VC2_CONTROL_DID_ENABLE |
	    VC2_CONTROL_CURSORFUNC_ENABLE |
	    VC2_CONTROL_CURSOR_ENABLE);

	/* Setup XMAP9s */
	xmap9_write(dc, XMAP9_DCBCRS_CONFIG,
	    XMAP9_CONFIG_8BIT_SYSTEM | XMAP9_CONFIG_RGBMAP_CI);

	xmap9_write(dc, XMAP9_DCBCRS_CURSOR_CMAP, 0);

	xmap9_write_mode(dc, 0,
	    XMAP9_MODE_GAMMA_BYPASS |
	    XMAP9_MODE_PIXSIZE_8BPP);
	xmap9_write(dc, XMAP9_DCBCRS_MODE_SELECT, 0);

	/* Setup REX3 */
	rex3_write(dc, REX3_REG_DRAWMODE1,
	    REX3_DRAWMODE1_PLANES_CI |
	    REX3_DRAWMODE1_DD_DD8 |
	    REX3_DRAWMODE1_RWPACKED |
	    REX3_DRAWMODE1_HD_HD8 |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    REX3_DRAWMODE1_LO_SRC);
	rex3_write(dc, REX3_REG_XYWIN, (4096 << 16) | 4096);
	rex3_write(dc, REX3_REG_TOPSCAN, 0x3ff); /* XXX Why? XXX */

	/* Setup CMAP */
	for (i=0; i<16; i++)
		newport_cmap_setrgb(dc, i, newport_defcmap[i*3],
		    newport_defcmap[i*3 + 1], newport_defcmap[i*3 + 2]);
}

/**** Attach routines ****/
static int
newport_match(struct device *parent, struct cfdata *self, void *aux)
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
}

static void
newport_attach(struct device *parent, struct device *self, void *aux)
{
	struct gio_attach_args *ga = aux;
	struct newport_softc *sc = (void *)self;
	struct wsemuldisplaydev_attach_args wa;

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

	wa.scrdata = &newport_screenlist;
	wa.accessops = &newport_accessops;
	wa.accesscookie = sc->sc_dc;

	config_found(&sc->sc_dev, &wa, wsemuldisplaydevprint);
}

int
newport_cnattach(struct gio_attach_args *ga)
{
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

	wsdisplay_cnattach(screen, &newport_console_dc, 0, 0, defattr);

	newport_is_console = 1;

	return 0;
}

/**** wsdisplay textops ****/
static void
newport_cursor(void *c, int on, int row, int col)
{
	struct newport_devconfig *dc = (void *)c;
	uint16_t control;
	int x_offset;

	control = vc2_read_ireg(dc, VC2_IREG_CONTROL);

	if (!on) {
		vc2_write_ireg(dc, VC2_IREG_CONTROL,
		    control & ~VC2_CONTROL_CURSOR_ENABLE);
	} else {
		/* Work around bug in some board revisions */
		if (dc->dc_vc2rev == 0) 
			x_offset = 29;
		else if (dc->dc_boardrev < 6)
			x_offset = 21;
		else
			x_offset = 31;

		vc2_write_ireg(dc, VC2_IREG_CURSOR_X,
		    col * dc->dc_fontdata->fontwidth + x_offset);
		vc2_write_ireg(dc, VC2_IREG_CURSOR_Y,
		    row * dc->dc_fontdata->fontheight + 31);
	
		vc2_write_ireg(dc, VC2_IREG_CONTROL,
		    control | VC2_CONTROL_CURSOR_ENABLE);
	}
}

static int
newport_mapchar(void *c, int ch, unsigned int *cp)
{
	struct newport_devconfig *dc = (void *)c;

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
	struct newport_devconfig *dc = (void *)c;
	struct wsdisplay_font *font = dc->dc_fontdata;
	uint8_t *bitmap = (u_int8_t *)font->data + (ch - font->firstchar) * 
	    font->fontheight * font->stride;
	uint32_t pattern;
	int i;
	int x = col * font->fontwidth;
	int y = row * font->fontheight;

	rex3_wait_gfifo(dc);
	
	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_DRAW |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_STOPONX |
	    REX3_DRAWMODE0_ENZPATTERN | REX3_DRAWMODE0_ZPOPAQUE);

	rex3_write(dc, REX3_REG_XYSTARTI, (x << REX3_XYSTARTI_XSHIFT) | y);
	rex3_write(dc, REX3_REG_XYENDI,
	    (x + font->fontwidth - 1) << REX3_XYENDI_XSHIFT);

	rex3_write(dc, REX3_REG_COLORI, NEWPORT_ATTR_FG(attr));
	rex3_write(dc, REX3_REG_COLORBACK, NEWPORT_ATTR_BG(attr));

	rex3_write(dc, REX3_REG_WRMASK, 0xffffffff);

	for (i=0; i<font->fontheight; i++) {
		/* XXX Works only with font->fontwidth == 8 XXX */
		pattern = *bitmap << 24;
		
		rex3_write_go(dc, REX3_REG_ZPATTERN, pattern);

		bitmap += font->stride;
	}
}

static void
newport_copycols(void *c, int row, int srccol, int dstcol, int ncols)
{
	struct newport_devconfig *dc = (void *)c;
	struct wsdisplay_font *font = dc->dc_fontdata;

	newport_copy_rectangle(dc,
	    srccol * font->fontwidth,			/* x1 */
	    row * font->fontheight,			/* y1 */
	    (srccol + ncols) * font->fontwidth - 1,	/* x2 */
	    (row + 1) * font->fontheight - 1,		/* y2 */
	    dstcol * font->fontheight,			/* dx */
	    row * font->fontheight);			/* dy */
}

static void
newport_erasecols(void *c, int row, int startcol, int ncols,
    long attr)
{
	struct newport_devconfig *dc = (void *)c;
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
	struct newport_devconfig *dc = (void *)c;
	struct wsdisplay_font *font = dc->dc_fontdata;

	newport_copy_rectangle(dc,
	    0,							/* x1 */
	    srcrow * font->fontheight,				/* y1 */
	    dc->dc_xres,					/* x2 */
	    (srcrow + nrows) * font->fontheight - 1,		/* y2 */
	    0,							/* dx */
	    dstrow * font->fontheight);				/* dy */
}

static void
newport_eraserows(void *c, int startrow, int nrows, long attr)
{
	struct newport_devconfig *dc = (void *)c;
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
newport_ioctl(void *c, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct newport_softc *sc = c;

#define FBINFO (*(struct wsdisplay_fbinfo*)data)

	switch (cmd) {
	case WSDISPLAYIO_GINFO:
		FBINFO.width  = sc->sc_dc->dc_xres;
		FBINFO.height = sc->sc_dc->dc_yres;
		FBINFO.depth  = sc->sc_dc->dc_depth;
		FBINFO.cmsize = 1 << FBINFO.depth;
		return 0;
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_NEWPORT;
		return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
newport_mmap(void *c, void *vs, off_t offset, int prot)
{
	struct newport_devconfig *dc = c;

	if ( offset >= 0xfffff)
		return -1;

	return mips_btop(dc->dc_addr + offset);
}

static int
newport_alloc_screen(void *c, const struct wsscreen_descr *type, void **cookiep,
    int *cursxp, int *cursyp, long *attrp)
{
	/* This won't get called for console screen and we don't support
	 * virtual screens */

	return ENOMEM;
}

static void
newport_free_screen(void *c, void *cookie)
{
	panic("newport_free_screen");
}
static int
newport_show_screen(void *c, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	return 0;
}
