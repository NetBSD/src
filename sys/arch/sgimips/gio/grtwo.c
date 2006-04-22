/* $NetBSD: grtwo.c,v 1.6.6.1 2006/04/22 11:37:55 simonb Exp $	 */

/*
 * Copyright (c) 2004 Christopher SEKIYA
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

/* wscons driver for SGI GR2 family of framebuffers
 * 
 * Heavily based on the newport wscons driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grtwo.c,v 1.6.6.1 2006/04/22 11:37:55 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wsfont/wsfont.h>

#include <sgimips/gio/giovar.h>
#include <sgimips/gio/grtwovar.h>
#include <sgimips/gio/grtworeg.h>

#include <sgimips/dev/int2var.h>

struct grtwo_softc {
	struct device   sc_dev;

	struct grtwo_devconfig *sc_dc;
};

struct grtwo_devconfig {
	u_int32_t        dc_addr;

	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	u_int8_t	boardrev;
	u_int8_t	backendrev;
	int             hq2rev;
	int             ge7rev;
	int             vc1rev;
	int             zbuffer;
	int             cmaprev;
	int             xmaprev;
	int             rexrev;
	int             xres;
	int             yres;
	int             depth;
	int             monitor;

	int             dc_font;
	struct wsdisplay_font *dc_fontdata;
};

static int      grtwo_match(struct device *, struct cfdata *, void *);
static void     grtwo_attach(struct device *, struct device *, void *);

CFATTACH_DECL(grtwo, sizeof(struct grtwo_softc),
	      grtwo_match, grtwo_attach, NULL, NULL);

/* textops */
static void     grtwo_cursor(void *, int, int, int);
static int      grtwo_mapchar(void *, int, unsigned int *);
static void     grtwo_putchar(void *, int, int, u_int, long);
static void     grtwo_copycols(void *, int, int, int, int);
static void     grtwo_erasecols(void *, int, int, int, long);
static void     grtwo_copyrows(void *, int, int, int);
static void     grtwo_eraserows(void *, int, int, long);
static int      grtwo_allocattr(void *, int, int, int, long *);

/* accessops */
static int      grtwo_ioctl(void *, void *, u_long, caddr_t, int, struct lwp *);
static paddr_t  grtwo_mmap(void *, void *, off_t, int);
static int
grtwo_alloc_screen(void *, const struct wsscreen_descr *,
		   void **, int *, int *, long *);
static void     grtwo_free_screen(void *, void *);
static int
                grtwo_show_screen(void *, void *, int, void (*) (void *, int, int), void *);

static int	grtwo_intr0(void *);
static int	grtwo_intr6(void *);

static const struct wsdisplay_emulops grtwo_textops = {
	.cursor = grtwo_cursor,
	.mapchar = grtwo_mapchar,
	.putchar = grtwo_putchar,
	.copycols = grtwo_copycols,
	.erasecols = grtwo_erasecols,
	.copyrows = grtwo_copyrows,
	.eraserows = grtwo_eraserows,
	.allocattr = grtwo_allocattr
};

static const struct wsdisplay_accessops grtwo_accessops = {
	.ioctl = grtwo_ioctl,
	.mmap = grtwo_mmap,
	.alloc_screen = grtwo_alloc_screen,
	.free_screen = grtwo_free_screen,
	.show_screen = grtwo_show_screen,
};

static const struct wsscreen_descr grtwo_screen = {
	.name = "1280x1024",
	.ncols = 160,
	.nrows = 64, /* 40 */
	.textops = &grtwo_textops,
	.fontwidth = 8,
	.fontheight = 16,
	.capabilities = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_REVERSE
};

static const struct wsscreen_descr *_grtwo_screenlist[] = {
	&grtwo_screen
};

static const struct wsscreen_list grtwo_screenlist = {
	sizeof(_grtwo_screenlist) / sizeof(struct wsscreen_descr *),
	_grtwo_screenlist
};

static struct grtwo_devconfig grtwo_console_dc;
static int      grtwo_is_console = 0;

#define GR2_ATTR_ENCODE(fg,bg)	(((fg) << 8) | (bg))
#define GR2_ATTR_BG(a)		((a) & 0xff)
#define GR2_ATTR_FG(a)		(((a) >> 8) & 0xff)

static const u_int16_t grtwo_cursor_data[128] = {
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

static const u_int8_t grtwo_defcmap[8 * 3] = {
	/* Normal colors */
	0x00, 0x00, 0x00,	/* black */
	0x7f, 0x00, 0x00,	/* red */
	0x00, 0x7f, 0x00,	/* green */
	0x7f, 0x7f, 0x00,	/* brown */
	0x00, 0x00, 0x7f,	/* blue */
	0x7f, 0x00, 0x7f,	/* magenta */
	0x00, 0x7f, 0x7f,	/* cyan */
	0xc7, 0xc7, 0xc7,	/* white - XXX too dim? */
};

static void 
grtwo_wait_gfifo(struct grtwo_devconfig * dc)
{
	int2_wait_fifo(1);
}

static inline void
grtwo_set_color(bus_space_tag_t iot, bus_space_handle_t ioh, int color)
{
	bus_space_write_4(iot, ioh, GR2_FIFO_COLOR, color);
}

/* Helper functions */
static void
grtwo_fill_rectangle(struct grtwo_devconfig * dc, int x1, int y1, int x2,
		     int y2, u_int8_t color)
{
	int remaining;
	int from_y;
	int to_y;

	/* gr2 sees coordinate 0,0 as the lower left corner, and 1279,1023
	   as the upper right.  To keep things consistent, we shall flip the
	   y axis. */

	/* There appears to be a limit to the number of vertical lines that we
	   can run through the the graphics engine at one go.  This probably has
	   something to do with vertical refresh.  Single-row fills are okay,
	   multiple-row screw up the board in exciting ways.  The copy_rectangle
	   workaround doesn't work for fills. */

	/* Coordinates, not length.  Remember that! */

	to_y = min(dc->yres - 1 - y1, dc->yres - 1 - y2);
	from_y = max(dc->yres - 1 - y1, dc->yres - 1 - y2);

	remaining = to_y - from_y;

	grtwo_wait_gfifo(dc);
	grtwo_set_color(dc->iot, dc->ioh, color);

	while (remaining) {
		if (remaining <= 32)
		{
			delay(10000);
			grtwo_wait_gfifo(dc);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_RECTI2D, x1);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, from_y);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, x2);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, from_y + remaining);
			break;
		} else {
			delay(100000);
			grtwo_wait_gfifo(dc);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_RECTI2D, x1);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, from_y);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, x2);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, from_y + remaining);
			from_y += 32;
			remaining -=32;
		}
	}
}

static void
grtwo_copy_rectangle(struct grtwo_devconfig * dc, int x1, int y1, int x2,
		     int y2, int width, int height)
{
	int             length = (width + 3) >> 2;
	int             lines = 4864 / length;
	int             from_y;
	int             to_y;
	int		temp_height;

	if ((y2 <= y1) || (height < lines)) {
		grtwo_wait_gfifo(dc);
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_RECTCOPY, length);
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, lines);
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, x1);
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, y1);
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, width);
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, height);
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, x2);
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, y2);
	} else {
		from_y = y1 + height - lines;
		to_y = y2 + height - lines;
		temp_height = MIN(height, lines);

		while (temp_height) {
			grtwo_wait_gfifo(dc);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_RECTCOPY, length);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, lines);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, x1);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, from_y);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, width);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, temp_height);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, x2);
			bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, to_y);
			height -= temp_height;
			height = MIN(height, lines);
			from_y -= temp_height;
			to_y -= temp_height;
		}
	}
}

static void
grtwo_cmap_setrgb(struct grtwo_devconfig * dc, int index, u_int8_t r, u_int8_t g, u_int8_t b)
{
	grtwo_wait_gfifo(dc);
	bus_space_write_1(dc->iot, dc->ioh, XMAPALL_ADDRHI,
			  ((index & 0x1f00) >> 8) );
	bus_space_write_1(dc->iot, dc->ioh, XMAPALL_ADDRLO,
			  (index & 0xff));
	bus_space_write_1(dc->iot, dc->ioh, XMAPALL_CLUT, r);
	bus_space_write_1(dc->iot, dc->ioh, XMAPALL_CLUT, g);
	bus_space_write_1(dc->iot, dc->ioh, XMAPALL_CLUT, b);
}

static void
grtwo_setup_hw(struct grtwo_devconfig * dc)
{
	int             i = 0;

	/* Get various revisions */
	dc->boardrev = (~(bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD0))) & GR2_REVISION_RD0_VERSION_MASK;

	/*
	 * boards prior to rev 4 have a pretty whacky config scheme.
         * what is doubly weird is that i have a rev 2 board, but the rev 4
	 * probe routines work just fine.
	 * we'll trust SGI, though, and separate things a bit.  it's only
	 * critical for the display depth calculation.
	 */

	if (dc->boardrev < 4) {
		dc->backendrev = ~(bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD2) & GR2_REVISION_RD2_BACKEND_REV) >> 2;
		if (dc->backendrev == 0)
			return;
		dc->zbuffer = !(bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD1) & GR2_REVISION_RD1_ZBUFFER);
		if ( (bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD3) & GR2_REVISION_RD3_VMA) != 3)
		  i++;
		if ( (bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD3) & GR2_REVISION_RD3_VMB) != 0x0c)
		  i++;
		if ( (bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD3) & GR2_REVISION_RD3_VMC) != 0x30)
		  i++;
		dc->depth = 8 * i;
		dc->monitor =
			((bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD2) & 0x03) << 1) |
			(bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD1) & 0x01);
	} else {
		dc->backendrev = ~(bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD1)) & 0x03;
		if (dc->backendrev == 0)
			return;
		dc->zbuffer = bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD1) & GR2_REVISION4_RD1_ZBUFFER;
		dc->depth = ((bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD1) & GR2_REVISION4_RD1_24BPP) >> 4) ? 24 : 8;
		dc->monitor = (bus_space_read_4(dc->iot, dc->ioh, GR2_REVISION_RD0) & GR2_REVISION4_RD0_MONITOR_MASK) >> 4;
	}

	dc->hq2rev = (bus_space_read_4(dc->iot, dc->ioh, HQ2_VERSION) & HQ2_VERSION_MASK) >> HQ2_VERSION_SHIFT;
	dc->ge7rev = (bus_space_read_4(dc->iot, dc->ioh, GE7_REVISION) & GE7_REVISION_MASK) >> 5;
	/* dc->vc1rev = vc1_read_ireg(dc, 5) & 0x07; */

	/* gr2 supports 1280x1024 only */
	dc->xres = 1280;
	dc->yres = 1024;

#if 0
	/* Setup cursor glyph */

	bus_space_write_4(dc->iot, dc->ioh, VC1_ADDRHI,
		(VC1_SRAM_CURSOR0_BASE >> 8) & 0xff);
	bus_space_write_4(dc->iot, dc->ioh, VC1_ADDRLO,
		VC1_SRAM_CURSOR0_BASE & 0xff);
	for (i = 0; i < 128; i++)
		bus_space_write_4(dc->iot, dc->ioh, VC1_SRAM, grtwo_cursor_data[i]);

	bus_space_write_4(dc->iot, dc->ioh, VC1_ADDRHI,
		(VC1_CURSOR_EP >> 8) & 0xff);
	bus_space_write_4(dc->iot, dc->ioh, VC1_ADDRLO,
		VC1_CURSOR_EP & 0xff);
	bus_space_write_4(dc->iot, dc->ioh, VC1_COMMAND, VC1_SRAM_CURSOR0_BASE);
	bus_space_write_4(dc->iot, dc->ioh, VC1_COMMAND, 0);
	bus_space_write_4(dc->iot, dc->ioh, VC1_COMMAND, 0);
	bus_space_write_4(dc->iot, dc->ioh, VC1_COMMAND, 0);

	/* Turn on cursor function, display, DID */
	bus_space_write_4(dc->iot, dc->ioh, VC1_SYSCTL,
		VC1_SYSCTL_VC1 | VC1_SYSCTL_DID |
		VC1_SYSCTL_CURSOR | VC1_SYSCTL_CURSOR_DISPLAY);
#endif
	
	/* Setup CMAP */
	for (i = 0; i < 8; i++)
		grtwo_cmap_setrgb(dc, i, grtwo_defcmap[i * 3],
			grtwo_defcmap[i * 3 + 1], grtwo_defcmap[i * 3 + 2]);
}

/* Attach routines */
static int
grtwo_match(struct device * parent, struct cfdata * self, void *aux)
{
	struct gio_attach_args *ga = aux;

	/*
	 * grtwo doesn't have anything that even vaguely resembles a product
	 * ID.  Instead, we determine presence by looking at the HQ2 "mystery"
	 * register, which contains a magic number.
	 */
	if ( badaddr((void *) (ga->ga_ioh + HQ2_MYSTERY), sizeof(u_int32_t)) )
		return 0;

	if ( (bus_space_read_4(ga->ga_iot, ga->ga_ioh, HQ2_MYSTERY)) != 0xdeadbeef)
		return 0;

	return 1;
}

static void
grtwo_attach_common(struct grtwo_devconfig * dc, struct gio_attach_args * ga)
{
	dc->dc_addr = ga->ga_addr;

	dc->iot = ga->ga_iot;
	dc->ioh = ga->ga_ioh;
	int i = 0;

	wsfont_init();

	dc->dc_font = wsfont_find(NULL, 8, 16, 0, WSDISPLAY_FONTORDER_L2R,
				  WSDISPLAY_FONTORDER_L2R);

	if (dc->dc_font < 0)
		panic("grtwo_attach_common: no suitable fonts");

	if (wsfont_lock(dc->dc_font, &dc->dc_fontdata))
		panic("grtwo_attach_common: unable to lock font data");

	grtwo_setup_hw(dc);

	/* Large fills are broken.  For now, clear the screen line-by-line. */
	for (i = 0; i < 64; i++)
		grtwo_eraserows(dc, i, 1, 0);

	/* If large fills worked, we'd do this instead:
	grtwo_fill_rectangle(dc, 0, 0, dc->xres - 1, dc->yres - 1, 0);
	*/
}

static void
grtwo_attach(struct device * parent, struct device * self, void *aux)
{
	struct gio_attach_args *ga = aux;
	struct grtwo_softc *sc = (void *) self;
	struct wsemuldisplaydev_attach_args wa;

	if (grtwo_is_console && ga->ga_addr == grtwo_console_dc.dc_addr) {
		wa.console = 1;
		sc->sc_dc = &grtwo_console_dc;
	} else {
		wa.console = 0;
		sc->sc_dc = malloc(sizeof(struct grtwo_devconfig),
				   M_DEVBUF, M_WAITOK | M_ZERO);
		if (sc->sc_dc == NULL)
			panic("grtwo_attach: out of memory");

		grtwo_attach_common(sc->sc_dc, ga);
	}

	aprint_naive(": Display adapter\n");

	aprint_normal(": GR2 (board rev %x, monitor %d, depth %d)\n",
	      sc->sc_dc->boardrev, sc->sc_dc->monitor, sc->sc_dc->depth);

	wa.scrdata = &grtwo_screenlist;
	wa.accessops = &grtwo_accessops;
	wa.accesscookie = sc->sc_dc;

        if ((cpu_intr_establish(0, IPL_TTY, grtwo_intr0, sc)) == NULL)
                printf(": unable to establish interrupt!\n");

        if ((cpu_intr_establish(6, IPL_TTY, grtwo_intr6, sc)) == NULL)
                printf(": unable to establish interrupt!\n");

	config_found(&sc->sc_dev, &wa, wsemuldisplaydevprint);
}

int
grtwo_cnattach(struct gio_attach_args * ga)
{
	long            defattr = GR2_ATTR_ENCODE(WSCOL_WHITE, WSCOL_BLACK);

	if (!grtwo_match(NULL, NULL, ga)) {
		return ENXIO;
	}

	grtwo_attach_common(&grtwo_console_dc, ga);
	wsdisplay_cnattach(&grtwo_screen, &grtwo_console_dc, 0, 0, defattr);

	grtwo_is_console = 1;

	return 0;
}

/* wsdisplay textops */
static void
grtwo_cursor(void *c, int on, int row, int col)
{
	struct grtwo_devconfig *dc = (void *) c;
	u_int32_t control;
	control = bus_space_read_4(dc->iot, dc->ioh, VC1_SYSCTL);

	if (!on) {
		bus_space_write_4(dc->iot, dc->ioh, VC1_SYSCTL,
			control & ~VC1_SYSCTL_CURSOR_DISPLAY);
	} else {
		bus_space_write_4(dc->iot, dc->ioh, VC1_ADDRHI, (VC1_CURSOR_XL & 0xff00) >> 8
			);
		bus_space_write_4(dc->iot, dc->ioh, VC1_ADDRLO, VC1_CURSOR_XL & 0xff);
		bus_space_write_4(dc->iot, dc->ioh, VC1_COMMAND,
				  col * dc->dc_fontdata->fontwidth);
		bus_space_write_4(dc->iot, dc->ioh, VC1_COMMAND,
				  row * dc->dc_fontdata->fontheight);
		bus_space_write_4(dc->iot, dc->ioh, VC1_SYSCTL,
			control | VC1_SYSCTL_CURSOR_DISPLAY);
	}
}

static int
grtwo_mapchar(void *c, int ch, unsigned int *cp)
{
	struct grtwo_devconfig *dc = (void *) c;

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
grtwo_putchar(void *c, int row, int col, u_int ch, long attr)
{
	struct grtwo_devconfig *dc = (void *) c;
	struct wsdisplay_font *font = dc->dc_fontdata;
	u_int8_t        *bitmap = (u_int8_t *) font->data + (ch - font->firstchar + 1) * font->fontheight * font->stride;
	u_int32_t        pattern;
	int             i;
	int             x = col * font->fontwidth;
	int             y = dc->yres - ( (row + 1) * font->fontheight);

	/* Set the drawing color */
	grtwo_wait_gfifo(dc);
	grtwo_set_color(dc->iot, dc->ioh, (((attr) >> 8) & 0xff));
	grtwo_wait_gfifo(dc);

	/* Set drawing coordinates */
	grtwo_wait_gfifo(dc);
	bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_CMOV2I, x);
	bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, y);

	/* This works for font sizes < 18 */
	grtwo_wait_gfifo(dc);
	bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DRAWCHAR, font->fontwidth);
	bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, font->fontheight);
	bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, 2);
	bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, 0); /* x offset */
	bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, 0); /* y offset */
	bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, 0);
	bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, 0);

	for (i = 0; i < font->fontheight; i++) {
		/* It appears that writes have to be 16 bits.  An "I tell you
		   two times" sort of thing?  Thanks, SGI */
		pattern = *bitmap | (*bitmap << 8);
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, pattern);
		bitmap -= font->stride;
	}

	/* pad up to 18 */
	for (i = font->fontheight; i < 18; i++)
		bus_space_write_4(dc->iot, dc->ioh, GR2_FIFO_DATA, 0x0000);
}

static void
grtwo_copycols(void *c, int row, int srccol, int dstcol, int ncols)
{
#if 1
	printf("grtwo_copycols: %i %i %i %i\n", row, srccol, dstcol, ncols);
#else
	struct grtwo_devconfig *dc = (void *) c;
	struct wsdisplay_font *font = dc->dc_fontdata;
	grtwo_copy_rectangle(dc,
			     srccol * font->fontwidth,	/* x1 */
			     0,	/* y1 */
			     dstcol * font->fontwidth,	/* x2 */
			     0,	/* y2 */
			     ncols * font->fontwidth,	/* dx */
			     dc->yres );	/* dy */
#endif
}

static void
grtwo_erasecols(void *c, int row, int startcol, int ncols, long attr)
{
	struct grtwo_devconfig *dc = (void *) c;
	struct wsdisplay_font *font = dc->dc_fontdata;

	grtwo_fill_rectangle(dc,
			     startcol * font->fontwidth,	/* x1 */
			     0,	/* y1 */
			     (startcol * font->fontwidth) + ncols * font->fontwidth,	/* x2 */
			     dc->yres,	/* y2 */
			     GR2_ATTR_BG(attr));
}

static void
grtwo_copyrows(void *c, int srcrow, int dstrow, int nrows)
{
	struct grtwo_devconfig *dc = (void *) c;
	struct wsdisplay_font *font = dc->dc_fontdata;

	grtwo_copy_rectangle(dc,
			     0,	/* x1 */
			     srcrow * font->fontheight,	/* y1 */
			     0, /* x2 */
			     dstrow * font->fontheight,	/* y2 */
			     dc->xres,	/* dx */
			     nrows * font->fontheight);
}

static void
grtwo_eraserows(void *c, int startrow, int nrows, long attr)
{
	struct grtwo_devconfig *dc = (void *) c;
	struct wsdisplay_font *font = dc->dc_fontdata;
	grtwo_fill_rectangle(dc,
			     0,	/* x1 */
			     startrow * font->fontheight,	/* y1 */
			     dc->xres,	/* x2 */
			     (startrow * font->fontheight) + nrows * font->fontheight,	/* y2 */
			     GR2_ATTR_BG(attr));
}

static int
grtwo_allocattr(void *c, int fg, int bg, int flags, long *attr)
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
		int             tmp = fg;
		fg = bg;
		bg = tmp;
	}
	*attr = GR2_ATTR_ENCODE(fg, bg);

	return 0;
}

/* wsdisplay accessops */

static int
grtwo_ioctl(void *c, void *vs, u_long cmd, caddr_t data, int flag,
	struct lwp *l)
{
	struct grtwo_softc *sc = c;

#define FBINFO (*(struct wsdisplay_fbinfo*)data)

	switch (cmd) {
	case WSDISPLAYIO_GINFO:
		FBINFO.width = sc->sc_dc->xres;
		FBINFO.height = sc->sc_dc->yres;
		FBINFO.depth = sc->sc_dc->depth;
		FBINFO.cmsize = 1 << FBINFO.depth;
		return 0;
	case WSDISPLAYIO_GTYPE:
		*(u_int *) data = WSDISPLAY_TYPE_GR2;
		return 0;
	}
	return EPASSTHROUGH;
}

static          paddr_t
grtwo_mmap(void *c, void *vs, off_t offset, int prot)
{
	struct grtwo_devconfig *dc = c;

	if (offset >= 0xfffff)
		return -1;

	return mips_btop(dc->dc_addr + offset);
}

static int
grtwo_alloc_screen(void *c, const struct wsscreen_descr * type, void **cookiep,
		   int *cursxp, int *cursyp, long *attrp)
{
	/*
	 * This won't get called for console screen and we don't support
	 * virtual screens
	 */

	return ENOMEM;
}

static void
grtwo_free_screen(void *c, void *cookie)
{
	panic("grtwo_free_screen");
}
static int
grtwo_show_screen(void *c, void *cookie, int waitok,
		  void (*cb) (void *, int, int), void *cbarg)
{
	return 0;
}

static int
grtwo_intr0(void *arg)
{
	/* struct grtwo_devconfig *dc = arg; */
	return 1;
}
	

static int
grtwo_intr6(void *arg)
{
	/* struct grtwo_devconfig *dc = arg; */
	return 1;
}
	
