/*	$NetBSD: amidisplaycc.c,v 1.5 2002/03/13 15:05:18 ad Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: amidisplaycc.c,v 1.5 2002/03/13 15:05:18 ad Exp $");

/*
 * wscons interface to amiga custom chips. Contains the necessary functions
 * to render text on bitmapped screens. Uses the functions defined in
 * grfabs_reg.h for display creation/destruction and low level setup.
 *
 * For each virtual terminal a new screen ('view') is allocated.
 * Also one more is allocated for the mapped screen on demand.
 */

#include "amidisplaycc.h"
#include "grfcc.h"
#include "view.h"
#include "opt_amigaccgrf.h"

#if NAMIDISPLAYCC>0

#include <sys/param.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <sys/conf.h>

#include <amiga/dev/grfabs_reg.h>
#include <amiga/dev/viewioctl.h>
#include <amiga/amiga/device.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rcons/raster.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/cons.h>
#include <dev/wsfont/wsfont.h>

#include <machine/stdarg.h>

#define AMIDISPLAYCC_MAXFONTS 8

/* These can be lowered if you are sure you dont need that much colors. */
#define MAXDEPTH 8
#define MAXROWS 128
#define MAXCOLUMNS 80

#define ADJUSTCOLORS

#define MAXCOLORS (1<<MAXDEPTH)

struct amidisplaycc_screen;
struct amidisplaycc_softc
{
	struct device dev;

	/* runtime-loaded fonts */
	struct wsdisplay_font         fonts[AMIDISPLAYCC_MAXFONTS];

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

static int  amidisplaycc_match(struct device *, struct cfdata *, void *);
static void amidisplaycc_attach(struct device *, struct device *, void *);

struct cfattach amidisplaycc_ca = {
	sizeof(struct amidisplaycc_softc),
	amidisplaycc_match,
	amidisplaycc_attach
};

cons_decl(amidisplaycc_);

/* end of configuration stuff */

/* private utility functions */

static int amidisplaycc_setvideo(struct amidisplaycc_softc *, int);

static int amidisplaycc_setemulcmap(struct amidisplaycc_screen *,
				    struct wsdisplay_cmap *);

static int amidisplaycc_cmapioctl(view_t *, u_long, struct wsdisplay_cmap *);
static int amidisplaycc_setcmap(view_t *, struct wsdisplay_cmap *);
static int amidisplaycc_getcmap(view_t *, struct wsdisplay_cmap *);
static int amidisplaycc_gfxscreen(struct amidisplaycc_softc *, int);

static int amidisplaycc_setnamedfont(struct amidisplaycc_screen *, char *);
static void amidisplaycc_setfont(struct amidisplaycc_screen *,
				 struct wsdisplay_font *, int);
static struct wsdisplay_font *amidisplaycc_findfont(struct amidisplaycc_softc *,
						    const char *, int, int);

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
int  amidisplaycc_alloc_attr(void *, int, int, int, long *);
/* end of emulops for wscons */


/* accessops for wscons */
int amidisplaycc_ioctl(void *, u_long, caddr_t, int, struct proc *);
paddr_t amidisplaycc_mmap(void *, off_t, int);
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
	amidisplaycc_alloc_attr
};

/* add some of our own data to the wsscreen_descr */
struct amidisplaycc_screen_descr {
	struct wsscreen_descr  wsdescr;
	int                    depth;
	char                   name[16];
};

/*
 * List of supported screenmodes. Almost anything can be given here.
 */

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

struct amidisplaycc_screen_descr amidisplaycc_screentab[] = {
	/* name, width, height, depth, fontwidth==8, fontheight */
	ADCC_SCREEN("80x50", 640, 400, 3, 8, 8),
	ADCC_SCREEN("80x40", 640, 400, 3, 8, 10),
	ADCC_SCREEN("80x25", 640, 400, 3, 8, 16),
	ADCC_SCREEN("80x24", 640, 384, 3, 8, 16),

	ADCC_SCREEN("640x400x1", 640, 400, 1, 8, 8),
	ADCC_SCREEN("640x400x2", 640, 400, 2, 8, 8),
	ADCC_SCREEN("640x400x3", 640, 400, 3, 8, 8),

	ADCC_SCREEN("640x200x1", 640, 200, 1, 8, 8),
	ADCC_SCREEN("640x200x1", 640, 200, 2, 8, 8),
	ADCC_SCREEN("640x200x1", 640, 200, 3, 8, 8),
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
};

#define NELEMS(arr) (sizeof(arr)/sizeof((arr)[0]))

/*
 * This structure also is passed to wscons. It contains pointers
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

	u_char  * savedscreen;

	/*
	 * The font is either one we loaded ourselves, or
	 * one gotten using the wsfont system.
	 *
	 * wsfontcookie differentiates between them:
	 * For fonts loaded by ourselves it is -1.
	 * For wsfonts it contains a cookie for that system.
	 */
	struct wsdisplay_font  * font;
	int                      wsfontcookie;
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
 * Bring in the one or two builtin fonts.
 */

extern unsigned char kernel_font_8x8[];
extern unsigned char kernel_font_lo_8x8;
extern unsigned char kernel_font_hi_8x8;

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
 * Of course pointers to this and other functions must present
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

	/* Yeah, we got the console! */

	/*
	 * This will do the basic stuff we also need.
	 */
	config_console();

	/*
	 * Call the init function in grfabs.c if we have
	 * no grfcc to do it.
	 * If grfcc is present it will call grfcc_probe()
	 * during config_console() above.
	 */
#if NGRFCC==0
	grfcc_probe();
#endif

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
}

static int
amidisplaycc_match(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	char *name = auxp;

	if (matchname("amidisplaycc", name) == 0)
		return (0);

	/* Allow only one of us now. Not sure about that. */
	if (cfp->cf_unit != 0)
		return (0);

	return 1;
}

/* ARGSUSED */
static void
amidisplaycc_attach(struct device *pdp, struct device *dp, void *auxp)
{
	struct wsemuldisplaydev_attach_args    waa;
	struct amidisplaycc_softc            * adp;

	adp = (struct amidisplaycc_softc*)dp;

	/*
	 * Attach only at real configuration time. Console init is done at
	 * the amidisplaycc_cninit function above.
	 */
	if (adp) {
		printf(": Amiga custom chip graphics %s",
		       aga_enable ? "(AGA)" : "");

		if (amidisplaycc_consolescreen.isconsole) {
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
		adp->gfxwidth = 640;
		adp->gfxheight = 480;

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
		waa.accesscookie = dp;
		config_found(dp, &waa, wsemuldisplaydevprint);

		bzero(adp->fonts, sizeof(adp->fonts));

		/* Initialize an alternate system for finding fonts. */
		wsfont_init();
	}
}


/*
 * Color, bgcolor and style are packed into one long attribute.
 * These macros are used to create/split the attribute
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
	u_char     * dst;
	int  i;

	scr = screen;

	if (row < 0 || col < 0 || row >= scr->nrows || col >= scr->ncols)
		return;

	if (!on && scr->cursorrow==-1 && scr->cursorcol==-1)
		return;

	if (!on) {
		row = scr->cursorrow;
		col = scr->cursorcol;
	}

	dst = scr->planes[0];
	dst += row * scr->rowbytes;
	dst += col;

	if (on) {
		scr->cursorrow = row;
		scr->cursorcol = col;
	} else {
		scr->cursorrow = -1;
		scr->cursorcol = -1;
	}

	for (i = scr->fontheight ; i > 0 ; i--) {
		*dst ^= 255;
		dst += scr->linebytes;
	}
}


/*
 * This obviously does something important, don't ask me what.
 */
int
amidisplaycc_mapchar(void *screen, int ch, unsigned int *chp)
{
	if (ch > 0 && ch < 256) {
		*chp = ch;
		return (5);
	}
	*chp = ' ';
	return (0);
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

	/* If we have loaded a font use it otherwise the builtin font */
	if (scr->font) {
		fontreal = scr->font->data;
		fontlow  = scr->font->firstchar;
		fonthigh = fontlow + scr->font->numchars - 1;
	} else {
		fontreal = kernel_font_8x8;
		fontlow  = kernel_font_lo_8x8;
		fonthigh = kernel_font_hi_8x8;
	}

	fontheight = scr->fontheight;
	depth      = scr->depth;
	linebytes  = scr->linebytes;

	if (ch < fontlow || ch > fonthigh)
		ch = fontlow;

	/* Find the location where the wanted char is in the font data */
	fontreal += scr->fontheight * (ch - fontlow);

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
 * Combined with eraserows it can be used to perform operation
 * also known as 'scrolling'.
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
					bzero(dst, copysize);
				} else {
					for (i = 0 ; i < fontheight ; i++) {
						bzero(dst, widthbytes);
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
}


/*
 * Compose an attribute value from foreground color,
 * background color, and flags.
 */
int
amidisplaycc_alloc_attr(void *screen, int fg, int bg, int flags, long *attrp)
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

	return (0);
}

int
amidisplaycc_ioctl(void *dp, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct amidisplaycc_softc *adp;

	adp = dp;

	if (adp == NULL) {
		printf("amidisplaycc_ioctl: adp==NULL\n");
		return (EINVAL);
	}

#define UINTDATA (*(u_int*)data)
#define INTDATA (*(int*)data)
#define FBINFO (*(struct wsdisplay_fbinfo*)data)

	switch (cmd)
	{
	case WSDISPLAYIO_GTYPE:
		UINTDATA = WSDISPLAY_TYPE_AMIGACC;
		return (0);

	case WSDISPLAYIO_SVIDEO:
		dprintf("amidisplaycc: WSDISPLAYIO_SVIDEO %s\n",
			UINTDATA ? "On" : "Off");

		return (amidisplaycc_setvideo(adp, UINTDATA));

	case WSDISPLAYIO_GVIDEO:
		dprintf("amidisplaycc: WSDISPLAYIO_GVIDEO\n");
		UINTDATA = adp->ison ?
		    WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;

		return (0);

	case WSDISPLAYIO_SMODE:
		if (INTDATA == WSDISPLAYIO_MODE_EMUL)
			return amidisplaycc_gfxscreen(adp, 0);
		if (INTDATA == WSDISPLAYIO_MODE_MAPPED)
			return amidisplaycc_gfxscreen(adp, 1);
		return (-1);

	case WSDISPLAYIO_GINFO:
		FBINFO.width  = adp->gfxwidth;
		FBINFO.height = adp->gfxheight;
		FBINFO.depth  = adp->gfxdepth;
		FBINFO.cmsize = 1 << FBINFO.depth;
		return (0);

	case WSDISPLAYIO_PUTCMAP:
	case WSDISPLAYIO_GETCMAP:
		return (amidisplaycc_cmapioctl(adp->gfxview,
					       cmd,
					       (struct wsdisplay_cmap*)data));
	}

	dprintf("amidisplaycc: unknown ioctl %lx (grp:'%c' num:%d)\n",
		(long)cmd,
		(char)((cmd&0xff00)>>8),
		(int)(cmd&0xff));

	return (-1);

#undef UINTDATA
#undef INTDATA
#undef FBINFO
}


/*
 * Switch to either emulation (text) or mapped (graphics) mode
 * We keep an extra screen for mapped mode so it does not
 * interfere with emulation screens.
 *
 * Once the extra screen is created, it never goes away.
 */

static int
amidisplaycc_gfxscreen(struct amidisplaycc_softc *adp, int on)
{
	dimen_t  dimension;

	dprintf("amidisplaycc: switching to %s mode.\n",
		on ? "mapped" : "emul");

	/* Current mode same as requested mode? */
	if ( (on > 0) == (adp->gfxon > 0) )
		return (0);

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

		return (0);
	}

	/* switch to mapped mode then */

	if (adp->gfxview == NULL) {
		/* First time here, create the screen */

		dimension.width = adp->gfxwidth;
		dimension.height = adp->gfxheight;

		dprintf("amidisplaycc: preparing mapped screen %dx%dx%d\n",
			dimension.width,
			dimension.height,
			adp->gfxdepth);

		adp->gfxview = grf_alloc_view(NULL,
					      &dimension,
					      adp->gfxdepth);
	}

	if (adp->gfxview) {
		adp->gfxon = 1;

		grf_display_view(adp->gfxview);
	} else {
		printf("amidisplaycc: failed to make mapped screen\n");
		return (ENOMEM);
	}
	return (0);
}

/*
 * Map the graphics screen. It must have been created before
 * by switching to mapped mode by using an ioctl.
 */
paddr_t
amidisplaycc_mmap(void *dp, off_t off, int prot)
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
	 * As we all know by now, we are mapping our special
	 * screen here so our pretty text consoles are left
	 * untouched.
	 */

	bm = adp->gfxview->bitmap;

	/* Check that the offset is valid */
	if (off < 0 || off >= bm->depth * bm->bytes_per_row * bm->rows) {
		dprintf("amidisplaycc_mmap: Offset out of range\n");
		return (paddr_t)(-1);
	}

	rv = (paddr_t)bm->hardware_address;
	rv += off;

	return (rv >> PGSHIFT);
}


/*
 * Create a new screen.
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
		return (ENOMEM);
	if (screenp->nrows > MAXROWS)
		return (ENOMEM);

	fontwidth = screenp->fontwidth;
	fontheight = screenp->fontheight;

	if (fontwidth != 8) {
		dprintf("amidisplaycc_alloc_screen: fontwidth %d invalid.\n",
		       fontwidth);
		return (EINVAL);
	}

	/*
	 * The screen size is defined in characters.
	 * Calculate the pixel size using the font size.
	 */

	dimension.width = screenp->ncols * fontwidth;
	dimension.height = screenp->nrows * fontheight;

	view = grf_alloc_view(NULL, &dimension, depth);
	if (view == NULL)
		return (ENOMEM);

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
		scr = malloc(sizeof(adccscr_t), M_DEVBUF, M_WAITOK);
		bzero(scr, sizeof(adccscr_t));
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


	/* --- LOAD FONT --- */

	/* these need to be initialized befory trying to set font */
	scr->font         = NULL;
	scr->wsfontcookie = -1;
	scr->fontwidth    = fontwidth;
	scr->fontheight   = fontheight;

	/*
	 * Note that dont try to load font for the console (adp==NULL)
	 *
	 * Here we dont care which font we get as long as it is the
	 * right size so pass NULL.
	 */
	if (adp)
		amidisplaycc_setnamedfont(scr, NULL);

	/*
	 * If no font found, use the builtin one.
	 * It will look stupid if the wanted size was different.
	 */
	if (scr->font == NULL) {
		scr->fontwidth = 8;
		scr->fontheight = min(8, fontheight);
	}

	/* --- LOAD FONT END --- */


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

	*curxp = 0;
	*curyp = 0;
	amidisplaycc_cursor(scr, 1, *curxp, *curyp);

	*defattrp = MAKEATTR(maxcolor, 0, 0);

	/* Show the console automatically */
	if (adp == NULL)
		grf_display_view(scr->view);

	if (adp) {
		dprintf("amidisplaycc: allocated screen; %dx%dx%d\n",
			dimension.width,
			dimension.height,
			depth);
	}

	return (0);
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
	adp = (struct amidisplaycc_softc*)adp;

	if (scr == NULL)
		return;

	/* Free the used font */
	amidisplaycc_setfont(scr, NULL, -1);

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
		return (EINVAL);
	}
	if (scr == NULL) {
		dprintf("amidisplaycc_show_screen: scr==NULL\n");
		return (EINVAL);
	}

	if (adp->gfxon) {
		dprintf("amidisplaycc: Screen shift while in gfx mode?");
		adp->gfxon = 0;
	}

	adp->currentscreen = scr;
	adp->ison = 1;

	grf_display_view(scr->view);

	return (0);
}

/*
 * Internal. Finds the font in our softc that has the desired attributes.
 * Or, if name is NULL, finds a free location for a new font.
 * Returns a pointer to font structure in softc or NULL for failure.
 *
 * Three possible forms:
 * findfont(adp, NULL, 0, 0)  -- find first empty location
 * findfont(adp, NULL, x, y)  -- find last font with given size
 * findfont(adp, name, x, y)  -- find last font with given name and size
 *
 * Note that when finding an empty location first one found is returned,
 * however when finding an existing font, the last one matching is
 * returned. This is because fonts cannot be unloaded and the last
 * font on the list is the one added latest and thus probably preferred.
 *
 * Note also that this is the only function which makes assumptions
 * about the storage location for the fonts.
 */
static struct wsdisplay_font *
amidisplaycc_findfont(struct amidisplaycc_softc *adp, const char *name,
		      int width, int height)
{
	struct wsdisplay_font  * font;

	int  findempty;
	int  f;

	if (adp == NULL) {
		dprintf("amidisplaycc_findfont: NULL adp\n");
		return NULL;
	}

	findempty = (name == NULL) && (width == 0) && (height == 0);

	font = NULL;

	for (f = 0 ; f < AMIDISPLAYCC_MAXFONTS ; f++) {

		if (findempty && adp->fonts[f].name == NULL)
			return &adp->fonts[f];

		if (!findempty && name == NULL && adp->fonts[f].name &&
		    adp->fonts[f].fontwidth == width &&
		    adp->fonts[f].fontheight == height)
			font = &adp->fonts[f];

		if (name && adp->fonts[f].name &&
		    strcmp(name, adp->fonts[f].name) == 0 &&
		    width == adp->fonts[f].fontwidth &&
		    height == adp->fonts[f].fontheight)
			font = &adp->fonts[f];
	}

	return (font);
}


/*
 * Set the font on a screen and free the old one.
 * Can be called with font of NULL to just free the
 * old one.
 * NULL font cannot be accompanied by valid cookie (!= -1)
 */
static void
amidisplaycc_setfont(struct amidisplaycc_screen *scr,
		     struct wsdisplay_font *font, int wsfontcookie)
{
	if (scr == NULL)
		panic("amidisplaycc_setfont: scr==NULL");
	if (font == NULL && wsfontcookie != -1)
		panic("amidisplaycc_setfont: no font but eat cookie");
	if (scr->font == NULL && scr->wsfontcookie != -1)
		panic("amidisplaycc_setfont: no font but eat old cookie");

	scr->font = font;

	if (scr->wsfontcookie != -1)
		wsfont_unlock(scr->wsfontcookie);

	scr->wsfontcookie = wsfontcookie;
}

/*
 * Try to find the named font and set the screen to use it.
 * Check both the fonts we have loaded with load_font and
 * fonts from wsfont system.
 *
 * Returns 0 on success.
 */

static int
amidisplaycc_setnamedfont(struct amidisplaycc_screen *scr, char *fontname)
{
	struct wsdisplay_font  * font;
	int  wsfontcookie;

	wsfontcookie = -1;

	if (scr == NULL || scr->device == NULL) {
		dprintf("amidisplaycc_setnamedfont: invalid\n");
		return (EINVAL);
	}

	/* Try first our dynamically loaded fonts. */
	font = amidisplaycc_findfont(scr->device,
				     fontname,
				     scr->fontwidth,
				     scr->fontheight);

	if (font == NULL) {
		/*
		 * Ok, no dynamically loaded font found.
		 * Try the wsfont system then.
		 */
		wsfontcookie = wsfont_find(fontname,
					   scr->fontwidth,
					   scr->fontheight,
					   1,
					   WSDISPLAY_FONTORDER_L2R,
					   WSDISPLAY_FONTORDER_L2R);

		if (wsfontcookie == -1)
			return (EINVAL);

		/* So, found a suitable font. Now lock it. */
		if (wsfont_lock(wsfontcookie,
				&font))
			return (EINVAL);

		/* Ok here we have the font successfully. */
	}

	amidisplaycc_setfont(scr, font, wsfontcookie);
	return (0);
}

/*
 * Load a font. This is used both to load a font and set it to screen.
 * The function depends on the parameters.
 * If the font has no data we must set a previously loaded
 * font with the same name. If it has data, then just load
 * the font but don't use it.
 */
int
amidisplaycc_load_font(void *dp, void *cookie, struct wsdisplay_font *font)
{
	struct amidisplaycc_softc   * adp;
	struct amidisplaycc_screen  * scr;
	struct wsdisplay_font       * myfont;

	u_int8_t  * c;
	void      * olddata;
	char      * name;

	u_int       size;
	u_int       i;


	adp = dp;
	scr = cookie;

	/*
	 * If font has no data it means we have to find the
	 * named font and use it.
	 */
	if (scr && font && font->name && !font->data)
		return amidisplaycc_setnamedfont(scr, font->name);


	/* Pre-load the font it is */

	if (font->stride != 1) {
		dprintf("amidisplaycc_load_font: stride %d != 1\n",
		       font->stride);
		return (-1);
	}

	if (font->fontwidth != 8) {
		dprintf("amidisplaycc_load_font: width %d not supported\n",
		       font->fontwidth);
		return (-1);
	}

	/* Size of the font in bytes... Assuming stride==1 */
	size = font->fontheight * font->numchars;

	/* Check if the same font was loaded before */
	myfont = amidisplaycc_findfont(adp,
				       font->name,
				       font->fontwidth,
				       font->fontheight);

	olddata = NULL;
	if (myfont) {
		/* Old font found, we will replace */

		if (myfont->name == NULL || myfont->data == NULL)
			panic("replacing NULL font/data");

		/*
		 * Store the old data pointer. We'll free it later
		 * when the new one is in place. Reallocation is needed
		 * because the new font may have a different number
		 * of characters in it than the last time it was loaded.
		 */

		olddata = myfont->data;

	} else {
		/* Totally brand new font */

		/* Try to find empty slot for the font */
		myfont = amidisplaycc_findfont(adp, NULL, 0, 0);

		if (myfont == NULL)
			return (ENOMEM);

		bzero(myfont, sizeof(struct wsdisplay_font));

		myfont->fontwidth = font->fontwidth;
		myfont->fontheight = font->fontheight;
		myfont->stride = font->stride;

		name = malloc(strlen(font->name)+1,
			      M_DEVBUF,
			      M_WAITOK);
		strcpy(name, font->name);
		myfont->name = name;
	}
	myfont->firstchar = font->firstchar;
	myfont->numchars  = font->numchars;

	myfont->data = malloc(size,
			      M_DEVBUF,
			      M_WAITOK);

	if (olddata)
		free(olddata, M_DEVBUF);


	memcpy(myfont->data, font->data, size);

	if (font->bitorder == WSDISPLAY_FONTORDER_R2L) {
		/* Reverse the characters. */
		c = myfont->data;
		for (i = 0 ; i < size ; i++) {
			*c = ((*c & 0x0f) << 4) | ((*c & 0xf0) >> 4);
			*c = ((*c & 0x33) << 2) | ((*c & 0xcc) >> 2);
			*c = ((*c & 0x55) << 1) | ((*c & 0xaa) >> 1);

			c++;
		}
	}

	/* Yeah, we made it */
	return (0);
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
		return (EINVAL);
	}
	if (adp->currentscreen == NULL) {
		dprintf("amidisplaycc_setvideo: adp->currentscreen==NULL\n");
		return (EINVAL);
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

	return (0);
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
		return (EINVAL);

	if (cmap->count == 0)
		return (0);

	tmpcmap.index = cmap->index;
	tmpcmap.count = cmap->count;
	tmpcmap.red   = cmred;
	tmpcmap.green = cmgrn;
	tmpcmap.blue  = cmblu;

	if (cmd == WSDISPLAYIO_PUTCMAP) {
		/* copy the color data to kernel space */

		err = copyin(cmap->red, cmred, cmap->count);
		if (err)
			return (err);

		err = copyin(cmap->green, cmgrn, cmap->count);
		if (err)
			return (err);

		err = copyin(cmap->blue, cmblu, cmap->count);
		if (err)
			return (err);

		return amidisplaycc_setcmap(view, &tmpcmap);

	} else if (cmd == WSDISPLAYIO_GETCMAP) {

		err = amidisplaycc_getcmap(view, &tmpcmap);
		if (err)
			return (err);

		/* copy data to user space */

		err = copyout(cmred, cmap->red, cmap->count);
		if (err)
			return (err);

		err = copyout(cmgrn, cmap->green, cmap->count);
		if (err)
			return (err);

		err = copyout(cmblu, cmap->blue, cmap->count);
		if (err)
			return (err);

		return (0);

	} else
		return (-1);
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
		return (rc);

	for (i = cmap->index ; i < cmap->index + cmap->count ; i++) {

		tmpcmap.red   [ scr->colormap[ i ] ] = cmap->red   [ i ];
		tmpcmap.green [ scr->colormap[ i ] ] = cmap->green [ i ];
		tmpcmap.blue  [ scr->colormap[ i ] ] = cmap->blue  [ i ];
	}

	rc = amidisplaycc_setcmap(scr->view, &tmpcmap);
	if (rc)
		return (rc);

	return (0);
}


/*
 * Set the colormap for the given screen.
 */

static int
amidisplaycc_setcmap(view_t *view, struct wsdisplay_cmap *cmap)
{
	u_long      cmentries [MAXCOLORS];

	int         green_div;
	int         blue_div;
	int         grey_div;
	int         red_div;
	u_int       colors;
	int         index;
	int         count;
	int         err;
	colormap_t  cm;
	int         c;

	if (view == NULL)
		return (EINVAL);

	if (!cmap || !cmap->red || !cmap->green || !cmap->blue) {
		dprintf("amidisplaycc_setcmap: other==NULL\n");
		return (EINVAL);
	}

	index  = cmap->index;
	count  = cmap->count;
	colors = (1 << view->bitmap->depth);

	if (count > colors || index >= colors || index + count > colors)
		return (EINVAL);

	if (count == 0)
		return (0);

	cm.entry = cmentries;
	cm.first = index;
	cm.size  = count;

	/*
	 * Get the old colormap. We need to do this at least to know
	 * how many bits to use with the color values.
	 */

	err = grf_get_colormap(view, &cm);
	if (err)
		return (err);

	/*
	 * The palette entries from wscons contain 8 bits per gun.
	 * We need to convert them to the number of bits the view
	 * expects. That is typically 4 or 8. Here we calculate the
	 * conversion constants with which we divide the color values.
	 */

	if (cm.type == CM_COLOR) {
		red_div = 256 / (cm.red_mask + 1);
		green_div = 256 / (cm.green_mask + 1);
		blue_div = 256 / (cm.blue_mask + 1);
	} else if (cm.type == CM_GREYSCALE)
		grey_div = 256 / (cm.grey_mask + 1);
	else
		return (EINVAL); /* Hmhh */

	/* Copy our new values to the current colormap */

	for (c = 0 ; c < count ; c++) {

		if (cm.type == CM_COLOR) {

			cm.entry[c + index] = MAKE_COLOR_ENTRY(
				cmap->red[c] / red_div,
				cmap->green[c] / green_div,
				cmap->blue[c] / blue_div);

		} else if (cm.type == CM_GREYSCALE) {

			/* Generate grey from average of r-g-b (?) */

			cm.entry[c + index] = MAKE_COLOR_ENTRY(
				0,
				0,
				(cmap->red[c] +
				 cmap->green[c] +
				 cmap->blue[c]) / 3 / grey_div);
		}
	}


	/*
	 * Now we have a new colormap that contains all the entries. Set
	 * it to the view.
	 */

	err = grf_use_colormap(view, &cm);
	if (err)
		return err;

	return (0);
}

/*
 * Return the colormap of the given screen.
 */

static int
amidisplaycc_getcmap(view_t *view, struct wsdisplay_cmap *cmap)
{
	u_long      cmentries [MAXCOLORS];

	int         green_mul;
	int         blue_mul;
	int         grey_mul;
	int         red_mul;
	u_int       colors;
	int         index;
	int         count;
	int         err;
	colormap_t  cm;
	int         c;

	if (view == NULL)
		return (EINVAL);

	if (!cmap || !cmap->red || !cmap->green || !cmap->blue)
		return (EINVAL);

	index  = cmap->index;
	count  = cmap->count;
	colors = (1 << view->bitmap->depth);

	if (count > colors || index >= colors || index + count > colors)
		return (EINVAL);

	if (count == 0)
		return (0);

	cm.entry = cmentries;
	cm.first = index;
	cm.size  = count;


	err = grf_get_colormap(view, &cm);
	if (err)
		return (err);

	if (cm.type == CM_COLOR) {
		red_mul   = 256 / (cm.red_mask + 1);
		green_mul = 256 / (cm.green_mask + 1);
		blue_mul  = 256 / (cm.blue_mask + 1);
	} else if (cm.type == CM_GREYSCALE) {
		grey_mul  = 256 / (cm.grey_mask + 1);
	} else
		return (EINVAL);

	/*
	 * Copy color data to wscons-style structure. Translate to
	 * 8 bits/gun from whatever resolution the color natively is.
	 */

	for (c = 0 ; c < count ; c++) {

		if (cm.type == CM_COLOR) {

			cmap->red[c]   = CM_GET_RED(cm.entry[index+c]);
			cmap->green[c] = CM_GET_GREEN(cm.entry[index+c]);
			cmap->blue[c]  = CM_GET_BLUE(cm.entry[index+c]);

			cmap->red[c]   *= red_mul;
			cmap->green[c] *= green_mul;
			cmap->blue[c]  *= blue_mul;

		} else if (cm.type == CM_GREYSCALE) {
			cmap->red[c]   = CM_GET_GREY(cm.entry[index+c]);
			cmap->red[c]  *= grey_mul;

			cmap->green[c] = cmap->red[c];
			cmap->blue[c]  = cmap->red[c];
		}
	}

	return (0);
}

/* ARGSUSED */
void
amidisplaycc_pollc(void *cookie, int on)
{
}

/*
 * These dummy functions are here just so that we can compete of
 * the console at init.
 * If we win the console then the wscons system will provide the
 * real ones which in turn will call the apropriate wskbd device.
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
	return (0);
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
