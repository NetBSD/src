/*	$NetBSD: amidisplaycc.c,v 1.39 2022/02/06 10:05:56 jandberg Exp $ */

/*-
 * Copyright (c) 2000 Jukka Andberg.
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
 *    derived from this software without specific prior written permission
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amidisplaycc.c,v 1.39 2022/02/06 10:05:56 jandberg Exp $");

/*
 * wscons interface to amiga custom chips. Contains the necessary functions
 * to render text on bitmapped screens. Uses the functions defined in
 * grfabs_reg.h for display creation/destruction and low level setup.
 *
 * For each virtual terminal a new screen (a grfabs view) is allocated.
 * Also one more view is allocated for the mapped screen on demand.
 */

#include "amidisplaycc.h"
#include "grfcc.h"
#include "view.h"
#include "opt_amigaccgrf.h"
#include "kbd.h"

#if NAMIDISPLAYCC>0

#include <sys/param.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <sys/conf.h>

#include <amiga/dev/grfabs_reg.h>
#include <amiga/dev/kbdvar.h>
#include <amiga/dev/viewioctl.h>
#include <amiga/amiga/device.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/cons.h>
#include <dev/wsfont/wsfont.h>

/* These can be lowered if you are sure you don't need that much colors. */
#define MAXDEPTH 8
#define MAXROWS 128

#define ADJUSTCOLORS

#define MAXCOLORS (1<<MAXDEPTH)

struct amidisplaycc_screen;
struct amidisplaycc_softc
{
	struct amidisplaycc_screen  * currentscreen;

	/* display turned on? */
	int       ison;

	/* stuff relating to the mapped screen */
	view_t  * gfxview;
	int       gfxwidth;
	int       gfxheight;
	int       gfxdepth;
	int       gfxon;
};


/*
 * Configuration stuff.
 */

static int  amidisplaycc_match(device_t, cfdata_t, void *);
static void amidisplaycc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(amidisplaycc, sizeof(struct amidisplaycc_softc),
    amidisplaycc_match, amidisplaycc_attach, NULL, NULL);

static int amidisplaycc_attached;

cons_decl(amidisplaycc_);

/* end of configuration stuff */

/* private utility functions */

static int amidisplaycc_setvideo(struct amidisplaycc_softc *, int);

static int amidisplaycc_setemulcmap(struct amidisplaycc_screen *,
				    struct wsdisplay_cmap *);

static int amidisplaycc_cmapioctl(view_t *, u_long, struct wsdisplay_cmap *);
static int amidisplaycc_setcmap(view_t *, struct wsdisplay_cmap *);
static int amidisplaycc_getcmap(view_t *, struct wsdisplay_cmap *);
static int amidisplaycc_setgfxview(struct amidisplaycc_softc *, int);
static void amidisplaycc_initgfxview(struct amidisplaycc_softc *);
static int amidisplaycc_getfbinfo(struct amidisplaycc_softc *, struct wsdisplayio_fbinfo *);

static int amidisplaycc_setfont(struct amidisplaycc_screen *, const char *);
static const struct wsdisplay_font * amidisplaycc_getbuiltinfont(void);
static void amidisplaycc_cursor_undraw(struct amidisplaycc_screen *);
static void amidisplaycc_cursor_draw(struct amidisplaycc_screen *);
static void amidisplaycc_cursor_xor(struct amidisplaycc_screen *, int, int);

static void dprintf(const char *fmt, ...);

/* end of private utility functions */

/* emulops for wscons */
void amidisplaycc_cursor(void *, int, int, int);
int amidisplaycc_mapchar(void *, int, unsigned int *);
void amidisplaycc_putchar(void *, int, int, u_int, long);
void amidisplaycc_copycols(void *, int, int, int, int);
void amidisplaycc_erasecols(void *, int, int, int, long);
void amidisplaycc_copyrows(void *, int, int, int);
void amidisplaycc_eraserows(void *, int, int, long);
int  amidisplaycc_allocattr(void *, int, int, int, long *);
/* end of emulops for wscons */


/* accessops for wscons */
int amidisplaycc_ioctl(void *, void *, u_long, void *, int, struct lwp *);
paddr_t amidisplaycc_mmap(void *, void *, off_t, int);
int amidisplaycc_alloc_screen(void *, const struct wsscreen_descr *, void **,
			      int *, int *, long *);
void amidisplaycc_free_screen( void *, void *);
int amidisplaycc_show_screen(void *, void *, int, void (*)(void *, int, int),
			     void *);
int amidisplaycc_load_font(void *, void *, struct wsdisplay_font *);
void amidisplaycc_pollc(void *, int);
/* end of accessops for wscons */

/*
 * These structures are passed to wscons, and they contain the
 * display-specific callback functions.
 */

const struct wsdisplay_accessops amidisplaycc_accessops = {
	amidisplaycc_ioctl,
	amidisplaycc_mmap,
	amidisplaycc_alloc_screen,
	amidisplaycc_free_screen,
	amidisplaycc_show_screen,
	amidisplaycc_load_font,
	amidisplaycc_pollc
};

const struct wsdisplay_emulops amidisplaycc_emulops = {
	amidisplaycc_cursor,
	amidisplaycc_mapchar,
	amidisplaycc_putchar,
	amidisplaycc_copycols,
	amidisplaycc_erasecols,
	amidisplaycc_copyrows,
	amidisplaycc_eraserows,
	amidisplaycc_allocattr
};

/* Add some of our own data to the wsscreen_descr */
struct amidisplaycc_screen_descr {
	struct wsscreen_descr  wsdescr;
	int                    depth;
};

#define ADCC_SCREEN(name, width, height, depth, fontwidth, fontheight) \
    /* CONSTCOND */ \
    {{ \
    name, \
    width / fontwidth, \
    height / fontheight, \
    &amidisplaycc_emulops, fontwidth, fontheight, \
    (depth > 1 ? WSSCREEN_WSCOLORS : 0) | WSSCREEN_REVERSE | \
    WSSCREEN_HILIT | WSSCREEN_UNDERLINE }, \
    depth }

/*
 * List of supported screen types.
 *
 * The first item in list is used for the console screen.
 * A suitable screen size is guessed for it by looking
 * at the GRF_* options.
 */
struct amidisplaycc_screen_descr amidisplaycc_screentab[] = {
	/* name, width, height, depth, fontwidth==8, fontheight */

#if defined(GRF_PAL) && !defined(GRF_NTSC)
	ADCC_SCREEN("default", 640, 512, 3, 8, 8),
#else
	ADCC_SCREEN("default", 640, 400, 3, 8, 8),
#endif
	ADCC_SCREEN("80x50", 640, 400, 3, 8, 8),
	ADCC_SCREEN("80x40", 640, 400, 3, 8, 10),
	ADCC_SCREEN("80x25", 640, 400, 3, 8, 16),
	ADCC_SCREEN("80x24", 640, 192, 3, 8, 8),

	ADCC_SCREEN("80x64", 640, 512, 3, 8, 8),
	ADCC_SCREEN("80x51", 640, 510, 3, 8, 10),
	ADCC_SCREEN("80x32", 640, 512, 3, 8, 16),
	ADCC_SCREEN("80x31", 640, 248, 3, 8, 8),

	ADCC_SCREEN("640x400x1", 640, 400, 1, 8, 8),
	ADCC_SCREEN("640x400x2", 640, 400, 2, 8, 8),
	ADCC_SCREEN("640x400x3", 640, 400, 3, 8, 8),

	ADCC_SCREEN("640x200x1", 640, 200, 1, 8, 8),
	ADCC_SCREEN("640x200x2", 640, 200, 2, 8, 8),
	ADCC_SCREEN("640x200x3", 640, 200, 3, 8, 8),
};

#define ADCC_SCREENPTR(index) &amidisplaycc_screentab[index].wsdescr
const struct wsscreen_descr *amidisplaycc_screens[] = {
	ADCC_SCREENPTR(0),
	ADCC_SCREENPTR(1),
	ADCC_SCREENPTR(2),
	ADCC_SCREENPTR(3),
	ADCC_SCREENPTR(4),
	ADCC_SCREENPTR(5),
	ADCC_SCREENPTR(6),
	ADCC_SCREENPTR(7),
	ADCC_SCREENPTR(8),
	ADCC_SCREENPTR(9),
	ADCC_SCREENPTR(10),
	ADCC_SCREENPTR(11),
	ADCC_SCREENPTR(12),
	ADCC_SCREENPTR(13),
	ADCC_SCREENPTR(14),
};

#define NELEMS(arr) (sizeof(arr)/sizeof((arr)[0]))

/*
 * This structure is passed to wscons. It contains pointers
 * to the available display modes.
 */

const struct wsscreen_list amidisplaycc_screenlist = {
	sizeof(amidisplaycc_screens)/sizeof(amidisplaycc_screens[0]),
	amidisplaycc_screens
};

/*
 * Our own screen structure. One will be created for each screen.
 */

struct amidisplaycc_screen
{
	struct amidisplaycc_softc *device;

	int       isconsole;
	int       isvisible;
	view_t  * view;

	int       ncols;
	int       nrows;

	int       cursorrow;
	int       cursorcol;
	int       cursordrawn;

	/* Active bitplanes for each character row. */
	int       rowmasks[MAXROWS];

	/* Mapping of colors to screen colors. */
	int       colormap[MAXCOLORS];

	/* Copies of display parameters for convenience */
	int       width;
	int       height;
	int       depth;

	int       widthbytes; /* bytes_per_row           */
	int       linebytes;  /* widthbytes + row_mod    */
	int       rowbytes;   /* linebytes * fontheight  */

	u_char  * planes[MAXDEPTH];

	const struct wsdisplay_font  * wsfont;
	int                      wsfontcookie; /* if -1, builtin font */
	int                      fontwidth;
	int                      fontheight;
};

typedef struct amidisplaycc_screen adccscr_t;

/*
 * Need one statically allocated screen for early init.
 * The rest are mallocated when needed.
 */
adccscr_t amidisplaycc_consolescreen;

/*
 * Default palettes for 2, 4 and 8 color emulation displays.
 */

/* black, grey */
static u_char pal2red[] = { 0x00, 0xaa };
static u_char pal2grn[] = { 0x00, 0xaa };
static u_char pal2blu[] = { 0x00, 0xaa };

/* black, red, green, grey */
static u_char pal4red[] = { 0x00, 0xaa, 0x00, 0xaa };
static u_char pal4grn[] = { 0x00, 0x00, 0xaa, 0xaa };
static u_char pal4blu[] = { 0x00, 0x00, 0x00, 0xaa };

/* black, red, green, brown, blue, magenta, cyan, grey */
static u_char pal8red[] = { 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa};
static u_char pal8grn[] = { 0x00, 0x00, 0xaa, 0xaa, 0x00, 0x00, 0xaa, 0xaa};
static u_char pal8blu[] = { 0x00, 0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa};

static struct wsdisplay_cmap pal2 = { 0, 2, pal2red, pal2grn, pal2blu };
static struct wsdisplay_cmap pal4 = { 0, 4, pal4red, pal4grn, pal4blu };
static struct wsdisplay_cmap pal8 = { 0, 8, pal8red, pal8grn, pal8blu };

#ifdef GRF_AGA
extern int aga_enable;
#else
static int aga_enable = 0;
#endif

/*
 * This gets called at console init to determine the priority of
 * this console device.
 *
 * Pointers to this and other functions must present
 * in constab[] in conf.c for this to work.
 */
void
amidisplaycc_cnprobe(struct consdev *cd)
{
	cd->cn_pri = CN_INTERNAL;

	/*
	 * Yeah, real nice. But if we win the console then the wscons system
	 * does the proper initialization.
	 */
	cd->cn_dev = NODEV;
}

/*
 * This gets called if this device is used as the console.
 */
void
amidisplaycc_cninit(struct consdev  * cd)
{
	void  * cookie;
	long    attr;
	int     x;
	int     y;

	/*
	 * This will do the basic stuff we also need.
	 */
	grfcc_probe();

#if NVIEW>0
	viewprobe();
#endif

	/*
	 * Set up wscons to handle the details.
	 * It will then call us back when it needs something
	 * display-specific. It will also set up cn_tab properly,
	 * something which we failed to do at amidisplaycc_cnprobe().
	 */

	/*
	 * The alloc_screen knows to allocate the first screen statically.
	 */
	amidisplaycc_alloc_screen(NULL, &amidisplaycc_screentab[0].wsdescr,
				  &cookie, &x, &y, &attr);
	wsdisplay_cnattach(&amidisplaycc_screentab[0].wsdescr,
			   cookie, x, y, attr);

#if NKBD>0
	/* tell kbd device it is used as console keyboard */
	kbd_cnattach();
#endif
}

static int
amidisplaycc_match(device_t parent, cfdata_t cf, void *aux)
{
	char *name = aux;

	if (matchname("amidisplaycc", name) == 0)
		return 0;

	/* Allow only one of us. */
	if (amidisplaycc_attached)
		return 0;

	return 1;
}

/* ARGSUSED */
static void
amidisplaycc_attach(device_t parent, device_t self, void *aux)
{
	struct wsemuldisplaydev_attach_args    waa;
	struct amidisplaycc_softc            * adp;

	amidisplaycc_attached = 1;

	adp = device_private(self);

	grfcc_probe();

#if NVIEW>0
	viewprobe();
#endif

	/*
	 * Attach only at real configuration time. Console init is done at
	 * the amidisplaycc_cninit function above.
	 */
	if (adp) {
		printf(": Amiga custom chip graphics %s",
		       aga_enable ? "(AGA)" : "");

		if (amidisplaycc_consolescreen.isconsole) {
			amidisplaycc_consolescreen.device = adp;
			adp->currentscreen = &amidisplaycc_consolescreen;
			printf(" (console)");
		} else
			adp->currentscreen = NULL;

		printf("\n");

		adp->ison = 1;

		/*
		 * Mapped screen properties.
		 * Would need a way to configure.
		 */
		adp->gfxview = NULL;
		adp->gfxon = 0;
		adp->gfxwidth = amidisplaycc_screentab[0].wsdescr.ncols *
			amidisplaycc_screentab[0].wsdescr.fontwidth;
		adp->gfxheight = amidisplaycc_screentab[0].wsdescr.nrows *
			amidisplaycc_screentab[0].wsdescr.fontheight;

		if (aga_enable)
			adp->gfxdepth = 8;
		else
			adp->gfxdepth = 4;

		if (NELEMS(amidisplaycc_screentab) !=
		    NELEMS(amidisplaycc_screens))
			panic("invalid screen definitions");

		waa.scrdata = &amidisplaycc_screenlist;
		waa.console = amidisplaycc_consolescreen.isconsole;
		waa.accessops = &amidisplaycc_accessops;
		waa.accesscookie = adp;
		config_found(self, &waa, wsemuldisplaydevprint, CFARGS_NONE);

		wsfont_init();
	}
}

/*
 * Foreground color, background color, and style are packed into one
 * long attribute. These macros are used to create/split the attribute.
 */

#define MAKEATTR(fg, bg, mode) (((fg)<<16) | ((bg)<<8) | (mode))
#define ATTRFG(attr) (((attr)>>16) & 255)
#define ATTRBG(attr) (((attr)>>8) & 255)
#define ATTRMO(attr) ((attr) & 255)

/*
 * Called by wscons to draw/clear the cursor.
 * We do this by xorring the block to the screen.
 *
 * This simple implementation will break if the screen is modified
 * under the cursor before clearing it.
 */
void
amidisplaycc_cursor(void *screen, int on, int row, int col)
{
	adccscr_t  * scr;

	scr = screen;

	if (row < 0 || col < 0 || row >= scr->nrows || col >= scr->ncols)
		return;

	amidisplaycc_cursor_undraw(scr);

	if (on) {
		scr->cursorrow = row;
		scr->cursorcol = col;
		amidisplaycc_cursor_draw(scr);
	} else {
		scr->cursorrow = -1;
		scr->cursorcol = -1;
	}
}

void
amidisplaycc_cursor_undraw(struct amidisplaycc_screen * scr)
{
	if (scr->cursordrawn) {
		amidisplaycc_cursor_xor(scr, scr->cursorrow, scr->cursorcol);
		scr->cursordrawn = 0;
	}
}

void
amidisplaycc_cursor_draw(struct amidisplaycc_screen * scr)
{
	if (!scr->cursordrawn && scr->cursorrow >= 0 && scr->cursorcol >= 0) {
		amidisplaycc_cursor_xor(scr, scr->cursorrow, scr->cursorcol);
		scr->cursordrawn = 1;
	}
}

void
amidisplaycc_cursor_xor(struct amidisplaycc_screen * scr, int row, int col)
{
	u_char * dst;
	int i;

	KASSERT(scr);
	KASSERT(row >= 0);
	KASSERT(col >= 0);

	dst = scr->planes[0];
	dst += row * scr->rowbytes;
	dst += col;

	for (i = scr->fontheight ; i > 0 ; i--) {
		*dst ^= 255;
		dst += scr->linebytes;
	}
}

int
amidisplaycc_mapchar(void *screen, int ch, unsigned int *chp)
{
	if (ch > 0 && ch < 256) {
		*chp = ch;
		return 5;
	}
	*chp = ' ';
	return 0;
}

/*
 * Write a character to screen with color / bgcolor / hilite(bold) /
 * underline / reverse.
 * Surely could be made faster but I'm not sure if its worth the
 * effort as scrolling is at least a magnitude slower.
 */
void
amidisplaycc_putchar(void *screen, int row, int col, u_int ch, long attr)
{
	adccscr_t  * scr;
	u_char     * dst;
	u_char     * font;

	int         fontheight;
	u_int8_t  * fontreal;
	int         fontlow;
	int         fonthigh;

	int     bmapoffset;
	int     linebytes;
	int     underline;
	int     fgcolor;
	int     bgcolor;
	int     plane;
	int     depth;
	int     mode;
	int     bold;
	u_char  f;
	int     j;

	scr = screen;

	if (row < 0 || col < 0 || row >= scr->nrows || col >= scr->ncols)
		return;

	/* Extract the colors from the attribute */
	fgcolor = ATTRFG(attr);
	bgcolor = ATTRBG(attr);
	mode    = ATTRMO(attr);

	/* Translate to screen colors */
	fgcolor = scr->colormap[fgcolor];
	bgcolor = scr->colormap[bgcolor];

	if (mode & WSATTR_REVERSE) {
		j = fgcolor;
		fgcolor = bgcolor;
		bgcolor = j;
	}

	bold      = (mode & WSATTR_HILIT) > 0;
	underline = (mode & WSATTR_UNDERLINE) > 0;

	fontreal = scr->wsfont->data;
	fontlow  = scr->wsfont->firstchar;
	fonthigh = fontlow + scr->wsfont->numchars - 1;

	fontheight = uimin(scr->fontheight, scr->wsfont->fontheight);
	depth      = scr->depth;
	linebytes  = scr->linebytes;

	if (ch < fontlow || ch > fonthigh)
		ch = fontlow;

	/* Find the location where the wanted char is in the font data */
	fontreal += scr->wsfont->fontheight * (ch - fontlow);

	bmapoffset = row * scr->rowbytes + col;

	scr->rowmasks[row] |= fgcolor | bgcolor;

	for (plane = 0 ; plane < depth ; plane++) {
		dst = scr->planes[plane] + bmapoffset;

		if (fgcolor & 1) {
			if (bgcolor & 1) {
				/* fg=on bg=on (fill) */

				for (j = 0 ; j < fontheight ; j++) {
					*dst = 255;
					dst += linebytes;
				}
			} else {
				/* fg=on bg=off (normal) */

				font = fontreal;
				for (j = 0 ; j < fontheight ; j++) {
					f = *(font++);
					f |= f >> bold;
					*dst = f;
					dst += linebytes;
				}

				if (underline)
					*(dst - linebytes) = 255;
			}
		} else {
			if (bgcolor & 1) {
				/* fg=off bg=on (inverted) */

				font = fontreal;
				for (j = 0 ; j < fontheight ; j++) {
					f = *(font++);
					f |= f >> bold;
					*dst = ~f;
					dst += linebytes;
				}

				if (underline)
					*(dst - linebytes) = 0;
			} else {
				/* fg=off bg=off (clear) */

				for (j = 0 ; j < fontheight ; j++) {
					*dst = 0;
					dst += linebytes;
				}
			}
		}
		fgcolor >>= 1;
		bgcolor >>= 1;
	}
}

/*
 * Copy characters on a row to another position on the same row.
 */

void
amidisplaycc_copycols(void *screen, int row, int srccol, int dstcol, int ncols)
{
	adccscr_t  * scr;
	u_char     * src;
	u_char     * dst;

	int  bmapoffset;
	int  linebytes;
	int  depth;
	int  plane;
	int  i;
	int  j;

	scr = screen;

	if (srccol < 0 || srccol + ncols > scr->ncols ||
	    dstcol < 0 || dstcol + ncols > scr->ncols ||
	    row < 0 || row >= scr->nrows)
		return;

	depth = scr->depth;
	linebytes = scr->linebytes;
	bmapoffset = row * scr->rowbytes;

	for (plane = 0 ; plane < depth ; plane++) {
		src = scr->planes[plane] + bmapoffset;

		for (j = 0 ; j < scr->fontheight ; j++) {
			dst = src;

			if (srccol < dstcol) {

				for (i = ncols - 1 ; i >= 0 ; i--)
					dst[dstcol + i] = src[srccol + i];

			} else {

				for (i = 0 ; i < ncols ; i++)
					dst[dstcol + i] = src[srccol + i];

			}
			src += linebytes;
		}
	}
}

/*
 * Erase part of a row.
 */

void
amidisplaycc_erasecols(void *screen, int row, int startcol, int ncols,
		       long attr)
{
	adccscr_t  * scr;
	u_char     * dst;

	int  bmapoffset;
	int  linebytes;
	int  bgcolor;
	int  depth;
	int  plane;
	int  fill;
	int  j;

	scr = screen;

	if (row < 0 || row >= scr->nrows ||
	    startcol < 0 || startcol + ncols > scr->ncols)
		return;

	depth = scr->depth;
	linebytes = scr->linebytes;
	bmapoffset = row * scr->rowbytes + startcol;

	/* Erase will be done using the set background color. */
	bgcolor = ATTRBG(attr);
	bgcolor = scr->colormap[bgcolor];

	for(plane = 0 ; plane < depth ; plane++) {

		fill = (bgcolor & 1) ? 255 : 0;

		dst = scr->planes[plane] + bmapoffset;

		for (j = 0 ; j < scr->fontheight ; j++) {
			memset(dst, fill, ncols);
			dst += linebytes;
		}
	}
}

/*
 * Copy a number of rows to another location on the screen.
 */

void
amidisplaycc_copyrows(void *screen, int srcrow, int dstrow, int nrows)
{
	adccscr_t  * scr;
	u_char     * src;
	u_char     * dst;

	int  srcbmapoffset;
	int  dstbmapoffset;
	int  widthbytes;
	int  fontheight;
	int  linebytes;
	u_int copysize;
	int  rowdelta;
	int  rowbytes;
	int  srcmask;
	int  dstmask;
	int  bmdelta;
	int  depth;
	int  plane;
	int  i;
	int  j;

	scr = screen;

	if (srcrow < 0 || srcrow + nrows > scr->nrows ||
	    dstrow < 0 || dstrow + nrows > scr->nrows)
		return;

	depth = scr->depth;

	widthbytes = scr->widthbytes;
	rowbytes   = scr->rowbytes;
	linebytes  = scr->linebytes;
	fontheight = scr->fontheight;

	srcbmapoffset = rowbytes * srcrow;
	dstbmapoffset = rowbytes * dstrow;

	if (srcrow < dstrow) {
		/* Move data downwards, need to copy from down to up */
		bmdelta = -rowbytes;
		rowdelta = -1;

		srcbmapoffset += rowbytes * (nrows - 1);
		srcrow += nrows - 1;

		dstbmapoffset += rowbytes * (nrows - 1);
		dstrow += nrows - 1;
	} else {
		/* Move data upwards, copy up to down */
		bmdelta = rowbytes;
		rowdelta = 1;
	}

	if (widthbytes == linebytes)
		copysize = rowbytes;
	else
		copysize = 0;

	for (j = 0 ; j < nrows ; j++) {
		/* Need to copy only planes that have data on src or dst */
		srcmask = scr->rowmasks[srcrow];
		dstmask = scr->rowmasks[dstrow];
		scr->rowmasks[dstrow] = srcmask;

		for (plane = 0 ; plane < depth ; plane++) {

			if (srcmask & 1) {
				/*
				 * Source row has data on this
				 * plane, copy it.
				 */

				src = scr->planes[plane] + srcbmapoffset;
				dst = scr->planes[plane] + dstbmapoffset;

				if (copysize > 0) {

					memcpy(dst, src, copysize);

				} else {

					/*
					 * Data not continuous,
					 * must do in pieces
					 */
					for (i=0 ; i < fontheight ; i++) {
						memcpy(dst, src, widthbytes);

						src += linebytes;
						dst += linebytes;
					}
				}
			} else if (dstmask & 1) {
				/*
				 * Source plane is empty, but dest is not.
				 * so all we need to is clear it.
				 */

				dst = scr->planes[plane] + dstbmapoffset;

				if (copysize > 0) {
					/* Do it all */
					memset(dst, 0, copysize);
				} else {
					for (i = 0 ; i < fontheight ; i++) {
						memset(dst, 0, widthbytes);
						dst += linebytes;
					}
				}
			}

			srcmask >>= 1;
			dstmask >>= 1;
		}
		srcbmapoffset += bmdelta;
		dstbmapoffset += bmdelta;

		srcrow += rowdelta;
		dstrow += rowdelta;
	}
}

/*
 * Erase some rows.
 */

void
amidisplaycc_eraserows(void *screen, int row, int nrows, long attr)
{
	adccscr_t  * scr;
	u_char     * dst;

	int  bmapoffset;
	int  fillsize;
	int  bgcolor;
	int  depth;
	int  plane;
	int  fill;
	int  j;

	int  widthbytes;
	int  linebytes;
	int  rowbytes;


	scr = screen;

	if (row < 0 || row + nrows > scr->nrows)
		return;
	amidisplaycc_cursor_undraw(scr);

	depth      = scr->depth;
	widthbytes = scr->widthbytes;
	linebytes  = scr->linebytes;
	rowbytes   = scr->rowbytes;

	bmapoffset = row * rowbytes;

	if (widthbytes == linebytes)
		fillsize = rowbytes * nrows;
	else
		fillsize = 0;

	bgcolor = ATTRBG(attr);
	bgcolor = scr->colormap[bgcolor];

	for (j = 0 ; j < nrows ; j++)
		scr->rowmasks[row+j] = bgcolor;

	for (plane = 0 ; plane < depth ; plane++) {
		dst = scr->planes[plane] + bmapoffset;
		fill = (bgcolor & 1) ? 255 : 0;

		if (fillsize > 0) {
			/* If the rows are continuous, write them all. */
			memset(dst, fill, fillsize);
		} else {
			for (j = 0 ; j < scr->fontheight * nrows ; j++) {
				memset(dst, fill, widthbytes);
				dst += linebytes;
			}
		}
		bgcolor >>= 1;
	}
	amidisplaycc_cursor_draw(scr);
}


/*
 * Compose an attribute value from foreground color,
 * background color, and flags.
 */
int
amidisplaycc_allocattr(void *screen, int fg, int bg, int flags, long *attrp)
{
	adccscr_t  * scr;
	int          maxcolor;
	int          newfg;
	int          newbg;

	scr = screen;
	maxcolor = (1 << scr->view->bitmap->depth) - 1;

	/* Ensure the colors are displayable. */
	newfg = fg & maxcolor;
	newbg = bg & maxcolor;

#ifdef ADJUSTCOLORS
	/*
	 * Hack for low-color screens, if background color is nonzero
	 * but would be displayed as one, adjust it.
	 */
	if (bg > 0 && newbg == 0)
		newbg = maxcolor;

	/*
	 * If foreground and background colors are different but would
	 * display the same fix them by modifying the foreground.
	 */
	if (fg != bg && newfg == newbg) {
		if (newbg > 0)
			newfg = 0;
		else
			newfg = maxcolor;
	}
#endif
	*attrp = MAKEATTR(newfg, newbg, flags);

	return 0;
}

int
amidisplaycc_ioctl(void *dp, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct amidisplaycc_softc *adp;

	adp = dp;

	if (adp == NULL) {
		printf("amidisplaycc_ioctl: adp==NULL\n");
		return EINVAL;
	}

#define UINTDATA (*(u_int*)data)
#define INTDATA (*(int*)data)
#define FBINFO (*(struct wsdisplay_fbinfo*)data)

	switch (cmd)
	{
	case WSDISPLAYIO_GTYPE:
		UINTDATA = WSDISPLAY_TYPE_AMIGACC;
		return 0;

	case WSDISPLAYIO_SVIDEO:
		dprintf("amidisplaycc: WSDISPLAYIO_SVIDEO %s\n",
			UINTDATA ? "On" : "Off");

		return amidisplaycc_setvideo(adp, UINTDATA);

	case WSDISPLAYIO_GVIDEO:
		dprintf("amidisplaycc: WSDISPLAYIO_GVIDEO\n");
		UINTDATA = adp->ison ?
		    WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;

		return 0;

	case WSDISPLAYIO_SMODE:
		switch (INTDATA) {
		case WSDISPLAYIO_MODE_EMUL:
			return amidisplaycc_setgfxview(adp, 0);
		case WSDISPLAYIO_MODE_MAPPED:
		case WSDISPLAYIO_MODE_DUMBFB:
			return amidisplaycc_setgfxview(adp, 1);
		default:
			return EINVAL;
		}

	case WSDISPLAYIO_GINFO:
		FBINFO.width  = adp->gfxwidth;
		FBINFO.height = adp->gfxheight;
		FBINFO.depth  = adp->gfxdepth;
		FBINFO.cmsize = 1 << FBINFO.depth;
		return 0;

	case WSDISPLAYIO_PUTCMAP:
	case WSDISPLAYIO_GETCMAP:
		return amidisplaycc_cmapioctl(adp->gfxview, cmd,
					       (struct wsdisplay_cmap*)data);
	case WSDISPLAYIO_GET_FBINFO:
		amidisplaycc_initgfxview(adp);
		return amidisplaycc_getfbinfo(adp, data);
	}

	return EPASSTHROUGH;

#undef UINTDATA
#undef INTDATA
#undef FBINFO
}

static int
amidisplaycc_getfbinfo(struct amidisplaycc_softc *adp, struct wsdisplayio_fbinfo *fbinfo)
{
	bmap_t *bm;

	KASSERT(adp);

	if (adp->gfxview == NULL) {
		return ENOMEM;
	}

	bm = adp->gfxview->bitmap;
	KASSERT(bm);

	/* Depth 1 since current X wsfb driver doesn't support multiple bitplanes */
	memset(fbinfo, 0, sizeof(*fbinfo));
	fbinfo->fbi_fbsize = bm->bytes_per_row * bm->rows;
	fbinfo->fbi_fboffset = 0;
	fbinfo->fbi_width = bm->bytes_per_row * 8;
	fbinfo->fbi_height = bm->rows;
	fbinfo->fbi_stride = bm->bytes_per_row;
	fbinfo->fbi_bitsperpixel = 1;
	fbinfo->fbi_pixeltype = WSFB_CI;
	fbinfo->fbi_flags = 0;
	fbinfo->fbi_subtype.fbi_cmapinfo.cmap_entries = 1 << adp->gfxdepth;

	return 0;
}

/*
 * Initialize (but not display) the view used for graphics.
 */
static void
amidisplaycc_initgfxview(struct amidisplaycc_softc *adp)
{
	dimen_t dimension;

	if (adp->gfxview == NULL) {
		/* First time here, create the screen */
		dimension.width = adp->gfxwidth;
		dimension.height = adp->gfxheight;
		adp->gfxview = grf_alloc_view(NULL,
			&dimension,
			adp->gfxdepth);
	}
}

/*
 * Switch to either emulation (text) or mapped (graphics) mode
 * We keep an extra screen for mapped mode so it does not
 * interfere with emulation screens.
 *
 * Once the extra screen is created, it never goes away.
 */
static int
amidisplaycc_setgfxview(struct amidisplaycc_softc *adp, int on)
{
	dprintf("amidisplaycc: switching to %s mode.\n",
		on ? "mapped" : "emul");

	/* Current mode same as requested mode? */
	if ( (on > 0) == (adp->gfxon > 0) )
		return 0;

	if (!on) {
		/*
		 * Switch away from mapped mode. If there is
		 * a emulation screen, switch to it, otherwise
		 * just try to hide the mapped screen.
		 */
		adp->gfxon = 0;
		if (adp->currentscreen)
			grf_display_view(adp->currentscreen->view);
		else if (adp->gfxview)
			grf_remove_view(adp->gfxview);

		return 0;
	}

	/* switch to mapped mode then */
	amidisplaycc_initgfxview(adp);

	if (adp->gfxview) {
		adp->gfxon = 1;

		grf_display_view(adp->gfxview);
	} else {
		printf("amidisplaycc: failed to make mapped screen\n");
		return ENOMEM;
	}
	return 0;
}

/*
 * Map the graphics screen. It must have been created before
 * by switching to mapped mode by using an ioctl.
 */
paddr_t
amidisplaycc_mmap(void *dp, void *vs, off_t off, int prot)
{
	struct amidisplaycc_softc  * adp;
	bmap_t                     * bm;
	paddr_t                      rv;

	adp = (struct amidisplaycc_softc*)dp;

	/* Check we are in mapped mode */
	if (adp->gfxon == 0 || adp->gfxview == NULL) {
		dprintf("amidisplaycc_mmap: Not in mapped mode\n");
		return (paddr_t)(-1);
	}

	/*
	 * Screen reserved for graphics is used to avoid writing
	 * over the text screens.
	 */

	bm = adp->gfxview->bitmap;

	/* Check that the offset is valid */
	if (off < 0 || off >= bm->depth * bm->bytes_per_row * bm->rows) {
		dprintf("amidisplaycc_mmap: Offset out of range\n");
		return (paddr_t)(-1);
	}

	rv = (paddr_t)bm->hardware_address;
	rv += off;

	return MD_BTOP(rv);
}


/*
 * Create a new screen.
 *
 * NULL dp signifies console and then memory is allocated statically
 * and the screen is automatically displayed.
 *
 * A font with suitable size is searched and if not found
 * the builtin 8x8 font is used.
 *
 * There are separate default palettes for 2, 4 and 8+ color
 * screens.
 */

int
amidisplaycc_alloc_screen(void *dp, const struct wsscreen_descr *screenp,
			  void  **cookiep, int *curxp, int *curyp,
			  long *defattrp)
{
	const struct amidisplaycc_screen_descr  * adccscreenp;
	struct amidisplaycc_screen              * scr;
	struct amidisplaycc_softc               * adp;
	view_t                                  * view;

	dimen_t  dimension;
	int      fontheight;
	int      fontwidth;
	int      maxcolor;
	int      depth;
	int      i;
	int      j;

	adccscreenp = (const struct amidisplaycc_screen_descr *)screenp;
	depth = adccscreenp->depth;

	adp = dp;

	maxcolor = (1 << depth) - 1;

	/* Sanity checks because of fixed buffers */
	if (depth > MAXDEPTH || maxcolor >= MAXCOLORS)
		return ENOMEM;
	if (screenp->nrows > MAXROWS)
		return ENOMEM;

	fontwidth = screenp->fontwidth;
	fontheight = screenp->fontheight;

	/*
	 * The screen size is defined in characters.
	 * Calculate the pixel size using the font size.
	 */

	dimension.width = screenp->ncols * fontwidth;
	dimension.height = screenp->nrows * fontheight;

	view = grf_alloc_view(NULL, &dimension, depth);
	if (view == NULL)
		return ENOMEM;

	/*
	 * First screen gets the statically allocated console screen.
	 * Others are allocated dynamically.
	 */
	if (adp == NULL) {
		scr = &amidisplaycc_consolescreen;
		if (scr->isconsole)
			panic("more than one console?");

		scr->isconsole = 1;
	} else {
		scr = malloc(sizeof(adccscr_t), M_DEVBUF, M_WAITOK|M_ZERO);
	}

	scr->view = view;

	scr->ncols = screenp->ncols;
	scr->nrows = screenp->nrows;

	/* Copies of most used values */
	scr->width  = dimension.width;
	scr->height = dimension.height;
	scr->depth  = depth;
	scr->widthbytes = view->bitmap->bytes_per_row;
	scr->linebytes  = scr->widthbytes + view->bitmap->row_mod;
	scr->rowbytes   = scr->linebytes * fontheight;

	scr->device = adp;


	/*
	 * Try to find a suitable font.
	 * Avoid everything but the builtin font for console screen.
	 * Builtin font is used if no other is found, even if it 
	 * has the wrong size.
	 */

	KASSERT(fontwidth == 8);

	scr->wsfont       = NULL;
	scr->wsfontcookie = -1;
	scr->fontwidth    = fontwidth;
	scr->fontheight   = fontheight;

	if (adp)
		amidisplaycc_setfont(scr, NULL);

	if (scr->wsfont == NULL)
	{
		scr->wsfont = amidisplaycc_getbuiltinfont();
		scr->wsfontcookie = -1;
	}

	KASSERT(scr->wsfont);
	KASSERT(scr->wsfont->stride == 1);

	for (i = 0 ; i < depth ; i++) {
		scr->planes[i] = view->bitmap->plane[i];
	}

	for (i = 0 ; i < MAXROWS ; i++)
		scr->rowmasks[i] = 0;

	/* Simple one-to-one mapping for most colors */
	for (i = 0 ; i < MAXCOLORS ; i++)
		scr->colormap[i] = i;

	/*
	 * Arrange the most used pens to quickest colors.
	 * The default color for given depth is (1<<depth)-1.
	 * It is assumed it is used most and it is mapped to
	 * color that can be drawn by writing data to one bitplane
	 * only.
	 * So map colors 3->2, 7->4, 15->8 and so on.
	 */
	for (i = 2 ; i < MAXCOLORS ; i *= 2) {
		j = i * 2 - 1;

		if (j < MAXCOLORS) {
			scr->colormap[i] = j;
			scr->colormap[j] = i;
		}
	}

	/*
	 * Set the default colormap.
	 */
	if (depth == 1)
		amidisplaycc_setemulcmap(scr, &pal2);
	else if (depth == 2)
		amidisplaycc_setemulcmap(scr, &pal4);
	else
		amidisplaycc_setemulcmap(scr, &pal8);

	*cookiep = scr;

	/* cursor initially at top left */
	scr->cursorrow = -1;
	scr->cursorcol = -1;
	*curxp = 0;
	*curyp = 0;
	amidisplaycc_cursor(scr, 1, *curxp, *curyp);

	*defattrp = MAKEATTR(maxcolor, 0, 0);

	/* Show the console automatically */
	if (adp == NULL)
		grf_display_view(scr->view);

	if (adp) {
		dprintf("amidisplaycc: allocated screen; %dx%dx%d; font=%s\n",
			dimension.width,
			dimension.height,
			depth,
			scr->wsfont->name);
	}

	return 0;
}


/*
 * Destroy a screen.
 */

void
amidisplaycc_free_screen(void *dp, void *screen)
{
	struct amidisplaycc_screen  * scr;
	struct amidisplaycc_softc   * adp;

	scr = screen;
	adp = (struct amidisplaycc_softc*)dp;

	if (scr == NULL)
		return;

	/* Free the used font */
	if (scr->wsfont && scr->wsfontcookie != -1)
		wsfont_unlock(scr->wsfontcookie);
	scr->wsfont = NULL;
	scr->wsfontcookie = -1;

	if (adp->currentscreen == scr)
		adp->currentscreen = NULL;

	if (scr->view)
		grf_free_view(scr->view);
	scr->view = NULL;

	/* Take care not to free the statically allocated console screen */
	if (scr != &amidisplaycc_consolescreen) {
		free(scr, M_DEVBUF);
	}
}

/*
 * Switch to another vt. Switch is always made immediately.
 */

/* ARGSUSED2 */
int
amidisplaycc_show_screen(void *dp, void *screen, int waitok,
			 void (*cb) (void *, int, int), void *cbarg)
{
	adccscr_t *scr;
	struct amidisplaycc_softc *adp;

	adp = (struct amidisplaycc_softc*)dp;
	scr = screen;

	if (adp == NULL) {
		dprintf("amidisplaycc_show_screen: adp==NULL\n");
		return EINVAL;
	}
	if (scr == NULL) {
		dprintf("amidisplaycc_show_screen: scr==NULL\n");
		return EINVAL;
	}

	if (adp->gfxon) {
		dprintf("amidisplaycc: Screen shift while in gfx mode?");
		adp->gfxon = 0;
	}

	adp->currentscreen = scr;
	adp->ison = 1;

	grf_display_view(scr->view);

	return 0;
}

/*
 * Load/set a font.
 *
 * Only setting is supported, as the wsfont pseudo-device can
 * handle the loading of fonts for us.
 */
int
amidisplaycc_load_font(void *dp, void *cookie, struct wsdisplay_font *font)
{
	struct amidisplaycc_softc   * adp __diagused;
	struct amidisplaycc_screen  * scr __diagused;

	adp = dp;
	scr = cookie;

	KASSERT(adp);
	KASSERT(scr);
	KASSERT(font);
	KASSERT(font->name);
	
	if (font->data)
	{
		/* request to load the font, not supported */
		return EINVAL;
	}
	else
	{
		/* request to use the given font on this screen */
		return amidisplaycc_setfont(scr, font->name);
	}
}

/*
 * Set display on/off.
 */
static int
amidisplaycc_setvideo(struct amidisplaycc_softc *adp, int mode)
{
        view_t * view;

	if (adp == NULL) {
		dprintf("amidisplaycc_setvideo: adp==NULL\n");
		return EINVAL;
	}
	if (adp->currentscreen == NULL) {
		dprintf("amidisplaycc_setvideo: adp->currentscreen==NULL\n");
		return EINVAL;
	}

	/* select graphics or emulation screen */
	if (adp->gfxon && adp->gfxview)
		view = adp->gfxview;
	else
		view = adp->currentscreen->view;

	if (mode) {
		/* on */

		grf_display_view(view);
		dprintf("amidisplaycc: video is now on\n");
		adp->ison = 1;

	} else {
		/* off */

		grf_remove_view(view);
		dprintf("amidisplaycc: video is now off\n");
		adp->ison = 0;
	}

	return 0;
}

/*
 * Handle the WSDISPLAY_[PUT/GET]CMAP ioctls.
 * Just handle the copying of data to/from userspace and
 * let the functions amidisplaycc_setcmap and amidisplaycc_putcmap
 * do the real work.
 */

static int
amidisplaycc_cmapioctl(view_t *view, u_long cmd, struct wsdisplay_cmap *cmap)
{
	struct wsdisplay_cmap  tmpcmap;
	u_char                 cmred[MAXCOLORS];
	u_char                 cmgrn[MAXCOLORS];
	u_char                 cmblu[MAXCOLORS];

	int                    err;

	if (cmap->index >= MAXCOLORS ||
	    cmap->count > MAXCOLORS ||
	    cmap->index + cmap->count > MAXCOLORS)
		return EINVAL;

	if (cmap->count == 0)
		return 0;

	tmpcmap.index = cmap->index;
	tmpcmap.count = cmap->count;
	tmpcmap.red   = cmred;
	tmpcmap.green = cmgrn;
	tmpcmap.blue  = cmblu;

	if (cmd == WSDISPLAYIO_PUTCMAP) {
		/* copy the color data to kernel space */

		err = copyin(cmap->red, cmred, cmap->count);
		if (err)
			return err;

		err = copyin(cmap->green, cmgrn, cmap->count);
		if (err)
			return err;

		err = copyin(cmap->blue, cmblu, cmap->count);
		if (err)
			return err;

		return amidisplaycc_setcmap(view, &tmpcmap);

	} else if (cmd == WSDISPLAYIO_GETCMAP) {

		err = amidisplaycc_getcmap(view, &tmpcmap);
		if (err)
			return err;

		/* copy data to user space */

		err = copyout(cmred, cmap->red, cmap->count);
		if (err)
			return err;

		err = copyout(cmgrn, cmap->green, cmap->count);
		if (err)
			return err;

		err = copyout(cmblu, cmap->blue, cmap->count);
		if (err)
			return err;

		return 0;

	} else
		return EPASSTHROUGH;
}

/*
 * Set the palette of a emulation screen.
 * Here we do only color remapping and then call
 * amidisplaycc_setcmap to do the work.
 */

static int
amidisplaycc_setemulcmap(struct amidisplaycc_screen *scr,
			 struct wsdisplay_cmap *cmap)
{
	struct wsdisplay_cmap  tmpcmap;

	u_char                 red [MAXCOLORS];
	u_char                 grn [MAXCOLORS];
	u_char                 blu [MAXCOLORS];

	int                    rc;
	int                    i;

	/*
	 * Get old palette first.
	 * Because of the color mapping going on in the emulation
	 * screen the color range may not be contiguous in the real
	 * palette.
	 * So get the whole palette, insert the new colors
	 * at the appropriate places and then set the whole
	 * palette back.
	 */

	tmpcmap.index = 0;
	tmpcmap.count = 1 << scr->depth;
	tmpcmap.red   = red;
	tmpcmap.green = grn;
	tmpcmap.blue  = blu;

	rc = amidisplaycc_getcmap(scr->view, &tmpcmap);
	if (rc)
		return rc;

	for (i = cmap->index ; i < cmap->index + cmap->count ; i++) {

		tmpcmap.red   [ scr->colormap[ i ] ] = cmap->red   [ i ];
		tmpcmap.green [ scr->colormap[ i ] ] = cmap->green [ i ];
		tmpcmap.blue  [ scr->colormap[ i ] ] = cmap->blue  [ i ];
	}

	rc = amidisplaycc_setcmap(scr->view, &tmpcmap);
	if (rc)
		return rc;

	return 0;
}


/*
 * Set the colormap for the given screen.
 */

static int
amidisplaycc_setcmap(view_t *view, struct wsdisplay_cmap *cmap)
{
	u_long      cmentries [MAXCOLORS];

	u_int       colors;
	int         index;
	int         count;
	int         err;
	colormap_t  cm;

	if (view == NULL)
		return EINVAL;

	if (!cmap || !cmap->red || !cmap->green || !cmap->blue) {
		dprintf("amidisplaycc_setcmap: other==NULL\n");
		return EINVAL;
	}

	index  = cmap->index;
	count  = cmap->count;
	colors = (1 << view->bitmap->depth);

	if (count > colors || index >= colors || index + count > colors)
		return EINVAL;

	if (count == 0)
		return 0;

	cm.entry = cmentries;
	cm.first = index;
	cm.size  = count;

	/*
	 * Get the old colormap. We need to do this at least to know
	 * how many bits to use with the color values.
	 */

	err = grf_get_colormap(view, &cm);
	if (err)
		return err;

	/*
	 * The palette entries from wscons contain 8 bits per gun.
	 * We need to convert them to the number of bits the view
	 * expects. That is typically 4 or 8. Here we calculate the
	 * conversion constants with which we divide the color values.
	 */

	if (cm.type == CM_COLOR) {
		int c, green_div, blue_div, red_div;
		
		red_div = 256 / (cm.red_mask + 1);
		green_div = 256 / (cm.green_mask + 1);
		blue_div = 256 / (cm.blue_mask + 1);
		
		for (c = 0 ; c < count ; c++)
			cm.entry[c + index] = MAKE_COLOR_ENTRY(
				cmap->red[c] / red_div,
				cmap->green[c] / green_div,
				cmap->blue[c] / blue_div);

	} else if (cm.type == CM_GREYSCALE) {
		int c, grey_div;

		grey_div = 256 / (cm.grey_mask + 1);

		/* Generate grey from average of r-g-b (?) */
		for (c = 0 ; c < count ; c++)
			cm.entry[c + index] = MAKE_COLOR_ENTRY(
				0,
				0,
				(cmap->red[c] +
				 cmap->green[c] +
				 cmap->blue[c]) / 3 / grey_div);
	} else
		return EINVAL; /* Hmhh */

	/*
	 * Now we have a new colormap that contains all the entries. Set
	 * it to the view.
	 */

	err = grf_use_colormap(view, &cm);
	if (err)
		return err;

	return 0;
}

/*
 * Return the colormap of the given screen.
 */

static int
amidisplaycc_getcmap(view_t *view, struct wsdisplay_cmap *cmap)
{
	u_long      cmentries [MAXCOLORS];

	u_int       colors;
	int         index;
	int         count;
	int         err;
	colormap_t  cm;

	if (view == NULL)
		return EINVAL;

	if (!cmap || !cmap->red || !cmap->green || !cmap->blue)
		return EINVAL;

	index  = cmap->index;
	count  = cmap->count;
	colors = (1 << view->bitmap->depth);

	if (count > colors || index >= colors || index + count > colors)
		return EINVAL;

	if (count == 0)
		return 0;

	cm.entry = cmentries;
	cm.first = index;
	cm.size  = count;

	err = grf_get_colormap(view, &cm);
	if (err)
		return err;

	/*
	 * Copy color data to wscons-style structure. Translate to
	 * 8 bits/gun from whatever resolution the color natively is.
	 */
	if (cm.type == CM_COLOR) {
		int c, red_mul, green_mul, blue_mul;
		
		red_mul   = 256 / (cm.red_mask + 1);
		green_mul = 256 / (cm.green_mask + 1);
		blue_mul  = 256 / (cm.blue_mask + 1);

		for (c = 0 ; c < count ; c++) {
			cmap->red[c] = red_mul *
				CM_GET_RED(cm.entry[index+c]);
			cmap->green[c] = green_mul *
				CM_GET_GREEN(cm.entry[index+c]);
			cmap->blue[c] = blue_mul *
				CM_GET_BLUE(cm.entry[index+c]);
		}
	} else if (cm.type == CM_GREYSCALE) {
		int c, grey_mul;

		grey_mul = 256 / (cm.grey_mask + 1);

		for (c = 0 ; c < count ; c++) {
			cmap->red[c] = grey_mul *
				CM_GET_GREY(cm.entry[index+c]);
			cmap->green[c] = grey_mul *
				CM_GET_GREY(cm.entry[index+c]);
			cmap->blue[c] = grey_mul *
				CM_GET_GREY(cm.entry[index+c]);
		}
	} else
		return EINVAL;

	return 0;
}

/*
 * Find and set a font for the given screen.
 *
 * If fontname is given, a font with that name and suitable
 * size (determined by the screen) is searched for.
 * If fontname is NULL, a font with suitable size is searched.
 *
 * On success, the found font is assigned to the screen and possible
 * old font is freed.
 */
static int
amidisplaycc_setfont(struct amidisplaycc_screen *scr, const char *fontname)
{
	struct wsdisplay_font *wsfont;
	int wsfontcookie;

	KASSERT(scr);

	wsfontcookie = wsfont_find(fontname,
		scr->fontwidth,
		scr->fontheight,
		1,
		WSDISPLAY_FONTORDER_L2R,
		WSDISPLAY_FONTORDER_L2R,
		WSFONT_FIND_BITMAP);

	if (wsfontcookie == -1)
		return EINVAL;

	/* Suitable font found. Now lock it. */
	if (wsfont_lock(wsfontcookie, &wsfont))
		return EINVAL;

	KASSERT(wsfont);

	if (scr->wsfont && scr->wsfontcookie != -1)
		wsfont_unlock(scr->wsfontcookie);

	scr->wsfont = wsfont;
	scr->wsfontcookie = wsfontcookie;

	return 0;
}

/*
 * Return a font that is guaranteed to exist.
 */
static const struct wsdisplay_font * 
amidisplaycc_getbuiltinfont(void)
{
	static struct wsdisplay_font font;
	
	extern unsigned char kernel_font_width_8x8;
	extern unsigned char kernel_font_height_8x8;
	extern unsigned char kernel_font_lo_8x8;
	extern unsigned char kernel_font_hi_8x8;
	extern unsigned char kernel_font_8x8[];

	font.name = "kf8x8";
	font.firstchar = kernel_font_lo_8x8;
	font.numchars = kernel_font_hi_8x8 - kernel_font_lo_8x8 + 1;
	font.fontwidth = kernel_font_width_8x8;
	font.stride = 1;
	font.fontheight = kernel_font_height_8x8;
	font.data = kernel_font_8x8;

	/* these values aren't really used for anything */
	font.encoding = WSDISPLAY_FONTENC_ISO;
	font.bitorder = WSDISPLAY_FONTORDER_KNOWN;
	font.byteorder = WSDISPLAY_FONTORDER_KNOWN;

	return &font;
}

/* ARGSUSED */
void
amidisplaycc_pollc(void *cookie, int on)
{
	if (amidisplaycc_consolescreen.isconsole)
	{
		if (on) 
		{
			/* About to use console, so make it visible */
			grf_display_view(amidisplaycc_consolescreen.view);
		}
		if (!on && 
		    amidisplaycc_consolescreen.isconsole && 
		    amidisplaycc_consolescreen.device != NULL && 
		    amidisplaycc_consolescreen.device->currentscreen != NULL) 
		{
			/* Restore the correct view after done with console use */
			grf_display_view(amidisplaycc_consolescreen.device->currentscreen->view);
		}
	}
}

/*
 * These dummy functions are here just so that we can compete of
 * the console at init.
 * If we win the console then the wscons system will provide the
 * real ones which in turn will call the appropriate wskbd device.
 * These should never be called.
 */

/* ARGSUSED */
void
amidisplaycc_cnputc(dev_t cd, int ch)
{
}

/* ARGSUSED */
int
amidisplaycc_cngetc(dev_t cd)
{
	return 0;
}

/* ARGSUSED */
void
amidisplaycc_cnpollc(dev_t cd, int on)
{
}


/*
 * Prints stuff if DEBUG is turned on.
 */

/* ARGSUSED */
static void
dprintf(const char *fmt, ...)
{
#ifdef DEBUG
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
#endif
}

#endif /* AMIDISPLAYCC */
