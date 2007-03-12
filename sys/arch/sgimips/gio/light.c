/*	$Id: light.c,v 1.4.6.1 2007/03/12 05:50:11 rmind Exp $	*/

/*
 * Copyright (c) 2006 Stephen M. Rumble
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

/*
 * SGI "Light" graphics, a.k.a. "Entry", "Starter", "LG1", and "LG2".
 *
 * 1024x768 8bpp at 60Hz.
 *
 * This driver supports the boards found in Indigo R3k and R4k machines.
 * There is a Crimson variant, but the register offsets differ significantly.
 *
 * Light's REX chip is the precursor of the REX3 found in "newport", hence
 * much similarity exists.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: light.c,v 1.4.6.1 2007/03/12 05:50:11 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/sysconf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wsfont/wsfont.h>

#include <sgimips/gio/giovar.h>
#include <sgimips/gio/lightvar.h>
#include <sgimips/gio/lightreg.h>

struct light_softc {
	struct device sc_dev;

	struct light_devconfig *sc_dc;
};

struct light_devconfig {
	uint32_t		dc_addr;

	bus_space_tag_t		dc_st;
	bus_space_handle_t	dc_sh;

	int			dc_boardrev;
	int                     dc_font;
	struct wsdisplay_font  *dc_fontdata;
};

/* always 1024x768x8 */
#define LIGHT_XRES	1024
#define LIGHT_YRES	768
#define LIGHT_DEPTH	8

static int	light_match(struct device *, struct cfdata *, void *);
static void	light_attach(struct device *, struct device *, void *);

CFATTACH_DECL(light, sizeof(struct light_softc), light_match, light_attach,
    NULL, NULL);

/* wsdisplay_emulops */
static void	light_cursor(void *, int, int, int);
static int	light_mapchar(void *, int, unsigned int *);
static void	light_putchar(void *, int, int, u_int, long);
static void	light_copycols(void *, int, int, int, int);
static void	light_erasecols(void *, int, int, int, long);
static void	light_copyrows(void *, int, int, int);
static void	light_eraserows(void *, int, int, long);
static int	light_allocattr(void *, int, int, int, long *);

/* wsdisplay_accessops */
static int	light_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	light_mmap(void *, void *, off_t, int);
static int	light_alloc_screen(void *, const struct wsscreen_descr *,
    void **, int *, int *, long *);
static void	light_free_screen(void *, void *);
static int	light_show_screen(void *, void *, int,
    void (*)(void *, int, int), void *);

static const struct wsdisplay_accessops light_accessops = {
	.ioctl		= light_ioctl,
	.mmap		= light_mmap,
	.alloc_screen	= light_alloc_screen,
	.free_screen	= light_free_screen,
	.show_screen	= light_show_screen,
	.load_font	= NULL,
	.pollc		= NULL,
	.scroll		= NULL
};

static const struct wsdisplay_emulops light_emulops = {
	.cursor		= light_cursor,
	.mapchar	= light_mapchar,
	.putchar	= light_putchar,
	.copycols	= light_copycols,
	.erasecols	= light_erasecols,
	.copyrows	= light_copyrows,
	.eraserows	= light_eraserows,
	.allocattr	= light_allocattr,
	.replaceattr	= NULL
};

static const struct wsscreen_descr light_screen = {
	.name           = "1024x768",
	.ncols          = 128,
	.nrows          = 48,
	.textops        = &light_emulops,
	.fontwidth      = 8,
	.fontheight     = 16,
	.capabilities   = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_REVERSE
};

const struct wsscreen_descr *_light_screenlist[] = {
	&light_screen
};

static const struct wsscreen_list light_screenlist = {
	sizeof(_light_screenlist) / sizeof(_light_screenlist[0]),
	_light_screenlist
};

static struct light_devconfig	light_console_dc;
static int			light_is_console = 0;

#define LIGHT_ATTR_ENCODE(fg, bg)	(((fg << 8) & 0xff00) | (bg * 0x00ff))
#define LIGHT_ATTR_FG(attr)		((attr >> 8) & 0x00ff)
#define LIGHT_ATTR_BG(attr)		(attr & 0x00ff)

#define LIGHT_IS_LG1(_rev)		((_rev) < 2)	/* else LG2 */

/*******************************************************************************
 * REX routines and helper functions
 ******************************************************************************/

static uint32_t
rex_read(struct light_devconfig *dc, uint32_t rset, uint32_t r)
{
	
	return (bus_space_read_4(dc->dc_st, dc->dc_sh, rset + r));
}

static void
rex_write(struct light_devconfig *dc, uint32_t rset, uint32_t r, uint32_t v)
{

	bus_space_write_4(dc->dc_st, dc->dc_sh, rset + r, v);
}

static uint8_t
rex_vc1_read(struct light_devconfig *dc)
{

	rex_write(dc, REX_PAGE1_GO, REX_P1REG_CFGSEL, REX_CFGSEL_VC1_SYSCTL);
	rex_read(dc, REX_PAGE1_GO, REX_P1REG_VC1_ADDRDATA);
	return (rex_read(dc, REX_PAGE1_SET, REX_P1REG_VC1_ADDRDATA));
}
		 
static void
rex_vc1_write(struct light_devconfig *dc, uint8_t val)
{

	rex_write(dc, REX_PAGE1_GO, REX_P1REG_CFGSEL, REX_CFGSEL_VC1_SYSCTL);
	rex_write(dc, REX_PAGE1_SET, REX_P1REG_VC1_ADDRDATA, val);
	rex_write(dc, REX_PAGE1_GO, REX_P1REG_VC1_ADDRDATA, val);
}

static void
rex_wait(struct light_devconfig *dc)
{

	while (rex_read(dc, REX_PAGE1_SET,REX_P1REG_CFGMODE) & REX_CFGMODE_BUSY)
		;
}

static int
rex_revision(struct light_devconfig *dc)
{

	rex_write(dc, REX_PAGE1_SET, REX_P1REG_CFGSEL, REX_CFGSEL_VC1_LADDR);
	rex_read(dc, REX_PAGE1_GO, REX_P1REG_WCLOCKREV);
	return (rex_read(dc, REX_PAGE1_SET, REX_P1REG_WCLOCKREV) & 0x7);
}

static void
rex_copy_rect(struct light_devconfig *dc, int from_x, int from_y, int to_x,
    int to_y, int width, int height)
{
	int dx, dy, ystarti, yendi;

	dx = from_x - to_x;
	dy = from_y - to_y;

	/* adjust for y. NB: STOPONX, STOPONY are inclusive */
	if (to_y > from_y) {
		ystarti = to_y + height - 1;
		yendi = to_y;
	} else {
		ystarti = to_y;
		yendi = to_y + height - 1;
	}

	rex_wait(dc);

	rex_write(dc, REX_PAGE0_SET, REX_P0REG_XSTARTI, to_x);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_XENDI, to_x + width);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_YSTARTI, ystarti);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_YENDI, yendi);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_COMMAND, REX_OP_DRAW |
	    REX_LOGICOP_SRC | REX_OP_FLG_LOGICSRC | REX_OP_FLG_QUADMODE |
	    REX_OP_FLG_BLOCK | REX_OP_FLG_STOPONX | REX_OP_FLG_STOPONY);
	rex_write(dc, REX_PAGE0_GO, REX_P0REG_XYMOVE,
	    ((dx << 16) & 0xffff0000) | (dy & 0x0000ffff));
}

static void
rex_fill_rect(struct light_devconfig *dc, int from_x, int from_y, int to_x,
    int to_y, long attr)
{

	rex_wait(dc);

	rex_write(dc, REX_PAGE0_SET, REX_P0REG_YSTARTI, from_y);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_YENDI, to_y);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_XSTARTI, from_x);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_XENDI, to_x);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_COLORREDI, LIGHT_ATTR_BG(attr));
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_COMMAND, REX_OP_DRAW |
	    REX_LOGICOP_SRC | REX_OP_FLG_QUADMODE | REX_OP_FLG_BLOCK |
	    REX_OP_FLG_STOPONX | REX_OP_FLG_STOPONY);
	rex_read(dc, REX_PAGE0_GO, REX_P0REG_COMMAND);
}

/*******************************************************************************
 * match/attach functions
 ******************************************************************************/

static int
light_match(struct device *parent, struct cfdata *self, void *aux)
{
	struct gio_attach_args *ga = aux;

	if (ga->ga_addr != LIGHT_ADDR_0 && ga->ga_addr != LIGHT_ADDR_1)
		return (0);

	if (platform.badaddr(
	    (void *)(ga->ga_ioh + REX_PAGE1_SET + REX_P1REG_XYOFFSET),
	    sizeof(uint32_t)))
		return (0);

	if (bus_space_read_4(ga->ga_iot, ga->ga_ioh,
	    REX_PAGE1_SET + REX_P1REG_XYOFFSET) != 0x08000800)
		return (0);

	return (1);
}

static void
light_attach_common(struct light_devconfig *dc, struct gio_attach_args *ga)
{

	dc->dc_addr = ga->ga_addr;
	dc->dc_st = ga->ga_iot;
	dc->dc_sh = ga->ga_ioh;

	dc->dc_boardrev = rex_revision(dc); 

	wsfont_init();

	dc->dc_font = wsfont_find(NULL, 8, 16, 0, WSDISPLAY_FONTORDER_L2R,
	    WSDISPLAY_FONTORDER_L2R);

	if (dc->dc_font < 0)
		panic("light_attach_common: no suitable fonts");

	if (wsfont_lock(dc->dc_font, &dc->dc_fontdata))
		panic("light_attach_common: unable to lock font data");

	rex_vc1_write(dc, rex_vc1_read(dc) & ~(VC1_SYSCTL_CURSOR |
	    VC1_SYSCTL_CURSOR_ON));
	rex_fill_rect(dc, 0, 0, LIGHT_XRES - 1, LIGHT_YRES - 1, 0);
}

static void
light_attach(struct device *parent, struct device *self, void *aux)
{
	struct gio_attach_args *ga = aux;
	struct light_softc *sc = (void *)self;
	struct wsemuldisplaydev_attach_args wa;

	if (light_is_console && ga->ga_addr == light_console_dc.dc_addr) {
		wa.console = 1;
		sc->sc_dc = &light_console_dc;
	} else {
		wa.console = 0;
		sc->sc_dc = malloc(sizeof(struct light_devconfig), M_DEVBUF,
		    M_WAITOK | M_ZERO);
		if (sc->sc_dc == NULL)
			panic("light_attach: out of memory");

		light_attach_common(sc->sc_dc, ga);
	}

	aprint_naive(": Display adapter\n");

	aprint_normal(": SGI LG%d (board revision %d)\n",
	    LIGHT_IS_LG1(sc->sc_dc->dc_boardrev) ? 1 : 2,
	    sc->sc_dc->dc_boardrev);

	wa.scrdata = &light_screenlist;
	wa.accessops = &light_accessops;
	wa.accesscookie = sc->sc_dc;

	config_found(&sc->sc_dev, &wa, wsemuldisplaydevprint);
}

int
light_cnattach(struct gio_attach_args *ga)
{

	if (!light_match(NULL, NULL, ga))
		return (ENXIO);

	light_attach_common(&light_console_dc, ga);

	wsdisplay_cnattach(&light_screen, &light_console_dc, 0, 0,
	    LIGHT_ATTR_ENCODE(WSCOL_WHITE, WSCOL_BLACK));

	light_is_console = 1;

	return (0);
}

/*******************************************************************************
 * wsdisplay_emulops
 ******************************************************************************/

static void
light_cursor(void *c, int on, int row, int col)
{
	/* XXX */
}

static int
light_mapchar(void *c, int ch, unsigned int *cp)
{
	struct light_devconfig *dc = (void *)c;

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
light_putchar(void *c, int row, int col, u_int ch, long attr)
{
        struct light_devconfig *dc = c;
	struct wsdisplay_font *font = dc->dc_fontdata;
	uint8_t *bitmap;
	uint32_t pattern;
	int i, x, y;

	bitmap = (u_int8_t *)font->data +
	    ((ch - font->firstchar) * font->fontheight * font->stride);
	x = col * font->fontwidth;
	y = row * font->fontheight;

	rex_wait(dc);

	rex_write(dc, REX_PAGE0_SET, REX_P0REG_YSTARTI, y);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_YENDI, y + font->fontheight - 1);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_XSTARTI, x);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_XENDI, x + font->fontwidth - 1);
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_COLORREDI, LIGHT_ATTR_FG(attr));
	rex_write(dc, REX_PAGE0_SET, REX_P0REG_COLORBACK, LIGHT_ATTR_BG(attr));
	rex_write(dc, REX_PAGE0_GO,  REX_P0REG_COMMAND, REX_OP_NOP);

	rex_wait(dc);

	rex_write(dc, REX_PAGE0_SET, REX_P0REG_COMMAND, REX_OP_DRAW |
	    REX_LOGICOP_SRC | REX_OP_FLG_ENZPATTERN | REX_OP_FLG_QUADMODE |
	    REX_OP_FLG_XYCONTINUE | REX_OP_FLG_STOPONX | REX_OP_FLG_BLOCK |
	    REX_OP_FLG_LENGTH32 | REX_OP_FLG_ZOPAQUE);

	for (i = 0; i < font->fontheight; i++) {
		/* XXX assumes font->fontwidth == 8 */
		pattern = *bitmap << 24;
		rex_write(dc, REX_PAGE0_GO, REX_P0REG_ZPATTERN, pattern); 
		bitmap += font->stride;
	}
}

/* copy set of columns within the same line */
static void
light_copycols(void *c, int row, int srccol, int dstcol, int ncols)
{
	struct light_devconfig *dc = c;
	struct wsdisplay_font *font = dc->dc_fontdata;
	int from_x, from_y, to_x, to_y, width, height;

	from_x	= srccol * font->fontwidth;
	from_y	= row * font->fontheight;
	to_x	= dstcol * font->fontwidth;
	to_y	= from_y;
	width	= ncols * font->fontwidth;
	height	= font->fontheight;
	
	rex_copy_rect(c, from_x, from_y, to_x, to_y, width, height);
}

/* erase a set of columns in the same line */
static void
light_erasecols(void *c, int row, int startcol, int ncols, long attr)
{
	struct light_devconfig *dc = c;
	struct wsdisplay_font *font = dc->dc_fontdata;
	int from_x, from_y, to_x, to_y;

	from_x	= startcol * font->fontwidth;
	from_y	= row * font->fontheight;
	to_x	= from_x + (ncols * font->fontwidth) - 1;
	to_y	= from_y + font->fontheight - 1;

	rex_fill_rect(c, from_x, from_y, to_x, to_y, attr);
}

/* copy a set of complete rows */
static void
light_copyrows(void *c, int srcrow, int dstrow, int nrows)
{
	struct light_devconfig *dc = c;
	struct wsdisplay_font *font = dc->dc_fontdata;
	int from_x, from_y, to_x, to_y, width, height;

	from_x	= 0;
	from_y	= srcrow * font->fontheight;
	to_x	= 0;
	to_y	= dstrow * font->fontheight;
	width	= LIGHT_XRES;
	height	= nrows * font->fontheight;

	rex_copy_rect(c, from_x, from_y, to_x, to_y, width, height);
}

/* erase a set of complete rows */
static void
light_eraserows(void *c, int row, int nrows, long attr)
{
	struct light_devconfig *dc = c;
	struct wsdisplay_font *font = dc->dc_fontdata;
	int from_x, from_y, to_x, to_y;

	from_x	= 0;
	from_y	= row * font->fontheight;
	to_x	= LIGHT_XRES - 1;
	to_y	= from_y + (nrows * font->fontheight) - 1;

	rex_fill_rect(c, from_x, from_y, to_x, to_y, attr);
}

static int
light_allocattr(void *c, int fg, int bg, int flags, long *attr)
{

	if (flags & ~(WSATTR_WSCOLORS | WSATTR_HILIT | WSATTR_REVERSE))
		return (EINVAL);

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

	*attr = LIGHT_ATTR_ENCODE(fg, bg);
	return (0);
}

/*******************************************************************************
 * wsdisplay_accessops
 ******************************************************************************/

static int
light_ioctl(void *c, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct wsdisplay_fbinfo *fbinfo = (struct wsdisplay_fbinfo *)data;

	switch (cmd) {
	case WSDISPLAYIO_GINFO:
		fbinfo->width	= LIGHT_XRES;
		fbinfo->height	= LIGHT_YRES;
		fbinfo->depth	= LIGHT_DEPTH;
		fbinfo->cmsize	= 1 << LIGHT_DEPTH;
		return (0);

	case WSDISPLAYIO_GMODE: 
		*(u_int *)data = WSDISPLAYIO_MODE_EMUL;
		break;

	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_LIGHT;
		return (0);

	case WSDISPLAYIO_SVIDEO:
		/*
		 * Turning off VC1 will stop refreshing the video ram (or so I
		 * suspect). We'll blank the screen after bringing it back up,
		 * since that's nicer than displaying garbage.
		 */
		if (*(u_int *)data == WSDISPLAYIO_VIDEO_OFF)
			rex_vc1_write(c,rex_vc1_read(c) & ~VC1_SYSCTL_VIDEO_ON);
		else {
			rex_vc1_write(c, rex_vc1_read(c) | VC1_SYSCTL_VIDEO_ON);
			rex_fill_rect(c, 0, 0, LIGHT_XRES-1, LIGHT_YRES-1, 0);
		}
		return (0);
	}

	return (EPASSTHROUGH);
}

static paddr_t
light_mmap(void *c, void *vs, off_t off, int prot)
{
        struct light_devconfig *dc = c;

	if (off >= 0x7fff)
		return (-1);

	return (mips_btop(dc->dc_addr + off));
}

static int
light_alloc_screen(void *c, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *attr)
{

	return (ENOMEM);
}

static void
light_free_screen(void *c, void *cookie)
{

	panic("light_free_screen");
}

static int
light_show_screen(void *c, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{

	return (0);
}
