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

/*
 * wscons interface to amiga custom chips. Contains the necessary functions
 * to render text on bitmapped screens. Uses the functions defined in 
 * grfabs_reg.h for display creation/destruction and low level setup.
 */

#include "amidisplaycc.h"
#include "grfcc.h"
#include "view.h"

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

struct amidisplaycc_softc
{
	struct device dev;
};


/* 
 * Configuration stuff.
 */

static int  amidisplaycc_match  __P((struct device *, 
				     struct cfdata *, 
				     void *));
static void amidisplaycc_attach __P((struct device *, 
				     struct device *, 
				     void *));

struct cfattach amidisplaycc_ca = {
	sizeof(struct amidisplaycc_softc), 
	amidisplaycc_match, 
	amidisplaycc_attach
};

cons_decl(amidisplaycc_);

/* end of configuration stuff */

/* These can be lowered if you are sure you dont need that much colors. */
#define MAXDEPTH 8
#define MAXCOLORS (1<<MAXDEPTH)
#define MAXROWS 128

/* Perform validity checking on parameters on some functions? */
#define PARANOIA

#define ADJUSTCOLORS

/* emulops for wscons */
void amidisplaycc_cursor       __P(( void *, int, int, int         ));
int  amidisplaycc_mapchar      __P(( void *, int, unsigned int *   ));
void amidisplaycc_putchar      __P(( void *, int, int, u_int, long ));
void amidisplaycc_copycols     __P(( void *, int, int, int, int    ));
void amidisplaycc_erasecols    __P(( void *, int, int, int, long   ));
void amidisplaycc_copyrows     __P(( void *, int, int, int         ));
void amidisplaycc_eraserows    __P(( void *, int, int, long        ));
int  amidisplaycc_alloc_attr   __P(( void *, int, int, int, long * ));
/* end of emulops for wscons */

/* accessops for wscons */
int      amidisplaycc_ioctl        __P(( void *, u_long, caddr_t, 
					 int, struct proc *              ));
paddr_t  amidisplaycc_mmap         __P(( void *, off_t, int              ));
int      amidisplaycc_alloc_screen __P(( void *, 
					 const struct wsscreen_descr *,
					 void **, int *, int *, long *   ));

void     amidisplaycc_free_screen  __P(( void *, void *                  ));
int      amidisplaycc_show_screen  __P(( void *, void *, int, 
					 void (*) (void *, int, int), 
					 void *                          ));
int      amidisplaycc_load_font    __P(( void *, void *, 
					 struct wsdisplay_font *         ));
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
	amidisplaycc_load_font
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
	struct wsscreen_descr wsdescr;
	int depth;
};

/*
 * List of supported screenmodes. Almost anything can be given here.
 */

#define ADCC_SCREEN(width,height,depth) { {#width "x" #height "x" #depth, width/8, height/8, &amidisplaycc_emulops, 8, 8, WSSCREEN_WSCOLORS|WSSCREEN_REVERSE|WSSCREEN_HILIT|WSSCREEN_UNDERLINE},depth }

struct amidisplaycc_screen_descr amidisplaycc_screentab[] = {
	ADCC_SCREEN(640,400,1),
	ADCC_SCREEN(640,400,2),
	ADCC_SCREEN(640,400,3),
	ADCC_SCREEN(640,400,4),

	ADCC_SCREEN(640,200,1),
	ADCC_SCREEN(640,200,2),
	ADCC_SCREEN(640,200,3),
	ADCC_SCREEN(640,200,4),

	ADCC_SCREEN(640,480,1),
	ADCC_SCREEN(640,480,2),
	ADCC_SCREEN(640,480,3),
	ADCC_SCREEN(640,480,4),
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
	ADCC_SCREENPTR(11)
};

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
	int isconsole;
	int isvisible;
	view_t *view;

	int ncols;
	int nrows;

	/* 
	 * For each row store which bitplanes contain 
	 * data (just optimisation) 
	 */
	int rowmasks[MAXROWS];

	/* Mapping of colors to screen colors. Currently mostly unused. */
	int colormap[MAXCOLORS];

	int fontheight;
};

typedef struct amidisplaycc_screen adccscr_t;

/* 
 * Need one statically allocated screen for early init.
 * The rest are mallocated when needed. 
 */
adccscr_t amidisplaycc_consolescreen;


/*
 * This gets called at console init to determine the priority of
 * this console device.
 *
 * Of course pointers to this and other functions must present
 * in constab[] in conf.c for this to work.
 */
void
amidisplaycc_cnprobe(cd)
	struct consdev *cd;
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
amidisplaycc_cninit(cd)
	struct consdev *cd;
{
	int x,y;
	void *cookie;
	long attr;

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

	amidisplaycc_alloc_screen(NULL, &amidisplaycc_screentab[0].wsdescr,
				  &cookie, &x, &y, &attr);
	wsdisplay_cnattach(&amidisplaycc_screentab[0].wsdescr,
			   cookie, x, y, attr);
}

int
amidisplaycc_match(pdp,cfp,auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{
	char *name = auxp;

	if (matchname("amidisplaycc",name)==0)
		return (0);

	/* Allow only one of us now. Not sure about that. */
	if (cfp->cf_unit != 0)
		return (0);

	return 1;
}

void
amidisplaycc_attach(pdp,dp,auxp)
	struct device *pdp;
	struct device *dp;
	void *auxp;
{
	struct wsemuldisplaydev_attach_args waa;

	/* 
	 * Attach only at real configuration time. Console init is done at
	 * the amidisplaycc_cninit function above.
	 */ 
	if (dp) {
		printf("\n");

		waa.console = 1;
		waa.scrdata = &amidisplaycc_screenlist;
		waa.accessops = &amidisplaycc_accessops;
		waa.accesscookie = NULL;
		config_found(dp,&waa,wsemuldisplaydevprint);
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
amidisplaycc_cursor(screen, on, row, col)
	void  *screen;
	int    on;
	int    row;
	int    col;
{
	adccscr_t *scr;
	u_char *plane;
	int y;
	int miny;
	int maxy;

	scr = screen;

#ifdef PARANOIA
	if (row < 0 || col < 0 || row >= scr->nrows || col >= scr->ncols)
		return;
#endif

	miny = row * scr->fontheight;
	maxy = miny + scr->fontheight;
       
	for (y = miny ; y < maxy ; y++) {
		plane = col + VDISPLAY_LINE(scr->view, 0, y);
		*plane ^= 255;
	}
    
}

/*
 * This obviously does something important, don't ask me what.
 */
int
amidisplaycc_mapchar(screen, ch, chp)
	void  *screen;
	int    ch;
	unsigned int *chp;
{
	if (ch > 0 && ch < 256) {
		*chp = ch;
		return (5);
	}
	*chp = ' ';
	return (0);
}

extern unsigned char kernel_font_8x8[];

/* Write a character to screen with color / bgcolor / hilite(bold) / 
 * underline / inverse.
 * Surely could be made faster but I'm not sure if its worth the
 * effort as scrolling is at least a magnitude slower.
 */
void
amidisplaycc_putchar(screen, row, col, ch, attr)
	void  *screen;
	int    row;
	int    col;
	u_int  ch;
	long   attr;
{
	bmap_t *bitmap;
	u_char *dst;
	adccscr_t *scr;
	u_char *font;
	u_char f;
	int j;
	int fgcolor;
	int bgcolor;
	int plane;
	int depth;
	int rowsize;
	int fontoffset;
	int bmapoffset;
	int mode;
	int bold;
	int underline;

	scr = screen;

#ifdef PARANOIA
	if (row < 0 || col < 0 || row >= scr->nrows || col >= scr->ncols)
		return;
#endif

	bitmap = scr->view->bitmap;
	depth = bitmap->depth;
	rowsize = bitmap->bytes_per_row + bitmap->row_mod;

	/* Extract the colors from the attribute */
	fgcolor = ATTRFG(attr);
	bgcolor = ATTRBG(attr);
	mode = ATTRMO(attr);

	/* Translate to screen colors */
	fgcolor = scr->colormap[fgcolor];
	bgcolor = scr->colormap[bgcolor];

	if(mode & WSATTR_REVERSE) {
		j = fgcolor;
		fgcolor = bgcolor;
		bgcolor = j;
	}

	if (mode & WSATTR_HILIT)
		bold = 1;
	else
		bold = 0;


	if (mode & WSATTR_UNDERLINE)
		underline = 1;
	else
		underline = 0;


	if (ch < 32)
		ch = 32;
	if (ch > 255)
		ch = 255;


	fontoffset = scr->fontheight * (ch - 32);
	bmapoffset = row * scr->fontheight * rowsize + col;

	scr->rowmasks[row] |= fgcolor | bgcolor;

	for (plane = 0 ; plane < depth ; plane++) {
		dst = bitmap->plane[plane] + bmapoffset; 
	
		if (fgcolor & 1) {
			if (bgcolor & 1) {
				/* fg=on bg=on (fill) */

				for (j = 0 ; j < scr->fontheight ; j++)
				{
					*dst = 255;
					dst += rowsize;
				}
			} else {
				/* fg=on bg=off (normal) */
			
				font = &kernel_font_8x8[fontoffset];
				for (j = 0 ; j < scr->fontheight ; j++)
				{
					f = *(font++);
					f |= f>>bold;
					*dst = f;
					dst += rowsize;
				}

				/* XXX underline does not recognise baseline */
				if (underline)
					*(dst-rowsize) = 255;
			}
		} else {
			if (bgcolor & 1) {
				/* fg=off bg=on (inverted) */
			
				font = &kernel_font_8x8[fontoffset];
				for (j = 0 ; j < scr->fontheight ; j++) {
					f = *(font++);
					f |= f>>bold;
					*dst = ~f;
					dst += rowsize;
				}

				/* XXX underline does not recognise baseline */
				if (underline)
					*(dst-rowsize) = 0;
			} else { 
				/* fg=off bg=off (clear) */

				for (j = 0 ; j < scr->fontheight ; j++) {
					*dst = 0;
					dst += rowsize;
				}
			}
		}
		fgcolor >>= 1;
		bgcolor >>= 1;
	}
}


void
amidisplaycc_copycols(screen, row, srccol, dstcol, ncols)
	void  *screen;
	int    row;
	int    srccol;
	int    dstcol;
	int    ncols;
{
	bmap_t *bitmap;
	adccscr_t *scr;
	int depth;
	int i;
	int j;
	int plane;
	int rowsize;
	u_char *buf;

	scr = screen;

#ifdef PARANOIA
	if (srccol < 0 || srccol + ncols > scr->ncols || 
	    dstcol < 0 || dstcol + ncols > scr->ncols ||
	    row < 0 || row >= scr->nrows)
		return;
#endif

	bitmap = scr->view->bitmap;
	depth = bitmap->depth;
	rowsize = bitmap->bytes_per_row + bitmap->row_mod;

	for (plane = 0 ; plane < depth ; plane++) {
		buf = bitmap->plane[plane] + row*scr->fontheight*rowsize;

		for (j = 0 ; j < scr->fontheight ; j++) {

			if (srccol < dstcol) {

				for (i = ncols - 1 ; i >= 0 ; i--)
					buf[dstcol + i] = buf[srccol + i];

			} else {

				for (i = 0 ; i < ncols ; i++)
					buf[dstcol + i] = buf[srccol + i];

			}
			buf += rowsize;
		}
	}
}

void 
amidisplaycc_erasecols(screen, row, startcol, ncols, attr)
	void  *screen;
	int    row;
	int    startcol;
	int    ncols;
	long   attr;
{
	bmap_t *bitmap;
	adccscr_t *scr;
	u_char *buf;
	int depth;
	int j;
	int plane;
	int rowsize;
	int fill;
	int bgcolor;

	scr = screen;

#ifdef PARANOIA
	if (row < 0 || row >= scr->nrows ||
	    startcol < 0 || startcol + ncols > scr->ncols)
		return;
#endif

	bitmap = scr->view->bitmap;
	depth = bitmap->depth;
	rowsize = bitmap->bytes_per_row + bitmap->row_mod;

	bgcolor = ATTRBG(attr);
	bgcolor = scr->colormap[bgcolor];

	for(plane = 0 ; plane < depth ; plane++) {

		fill = (bgcolor & 1) ? 255 : 0;

		buf = bitmap->plane[plane];
		buf += row * scr->fontheight * rowsize;
		buf += startcol;

		for (j = 0 ; j < scr->fontheight ; j++)
		{
			memset(buf, fill, ncols);
			buf += rowsize;
		}
	}
}

void
amidisplaycc_copyrows(screen, srcrow, dstrow, nrows)
	void  *screen;
	int    srcrow;
	int    dstrow;
	int    nrows;
{
	bmap_t *bitmap;
	adccscr_t *scr;
	u_char *src;
	u_char *dst;

	int depth;
	int plane;
	int i;
	int j;
	int rowmod;
	int bytesperrow;
	int rowsize;
	int srcmask;
	int dstmask;
	int srcbmapoffset;
	int dstbmapoffset;
	int copysize;
	int bmdelta;
	int rowdelta;
	int bmwidth;

	scr = screen;

#ifdef PARANOIA
	if (srcrow < 0 || srcrow + nrows > scr->nrows ||
	    dstrow < 0 || dstrow + nrows > scr->nrows)
		return;
#endif

	bitmap = scr->view->bitmap;
	depth = bitmap->depth;
	rowmod = bitmap->row_mod;
	bytesperrow = bitmap->bytes_per_row;
	bmwidth = bytesperrow+rowmod;
	rowsize = bmwidth*scr->fontheight;

	srcbmapoffset = rowsize*srcrow;
	dstbmapoffset = rowsize*dstrow;

	if (srcrow < dstrow) {
		/* Move data downwards, need to copy from down to up */
		bmdelta = -rowsize;
		rowdelta = -1;

		srcbmapoffset += rowsize * (nrows - 1);
		srcrow += nrows - 1;

		dstbmapoffset += rowsize * (nrows - 1);
		dstrow += nrows - 1;
	} else {
		/* Move data upwards, copy up to down */
		bmdelta = rowsize;
		rowdelta = 1;
	}

	if (rowmod == 0)
		copysize = rowsize;
	else
		copysize = 0;

	for (j = 0 ; j < nrows ; j++)    {
		/* Need to copy only planes that have data on src or dst */
		srcmask = scr->rowmasks[srcrow];
		dstmask = scr->rowmasks[dstrow];
		scr->rowmasks[dstrow] = srcmask;

		for (plane = 0 ; plane < depth ; plane++) {

			if (srcmask & 1) {
				/* 
				 * Source row has data on this 
				 * plane, copy it 
				 */

				src = bitmap->plane[plane] + srcbmapoffset;
				dst = bitmap->plane[plane] + dstbmapoffset;

				if (copysize > 0) {

					/* Do it all */
					memcpy(dst,src,copysize);

				} else {

					/* 
					 * Data not continuous, 
					 * must do in pieces 
					 */
					for (i=0 ; i < scr->fontheight ; i++) {
						memcpy(dst, src, bytesperrow);

						src += bmwidth;
						dst += bmwidth;
					}
				}
			} else if (dstmask & 1) {
				/* 
				 * Source plane is empty, but dest is not.
				 * so all we need to is clear it.
				 */

				dst = bitmap->plane[plane] + dstbmapoffset;

				if (copysize > 0) {
					/* Do it all */
					bzero(dst, copysize);
				} else {
					for (i = 0 ; 
					     i < scr->fontheight ; i++) {
						bzero(dst, bytesperrow);
						dst += bmwidth;
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

void
amidisplaycc_eraserows(screen, row, nrows, attr)
	void  *screen;
	int    row;
	int    nrows;
	long   attr;
{
	bmap_t *bitmap;
	adccscr_t *scr;
	int depth;
	int plane;
	int j;
	int bytesperrow;
	int rowmod;
	int rowsize;
	int bmwidth;
	int bgcolor;
	int fill;
	int bmapoffset;
	int fillsize;
	u_char *dst;

	scr = screen;

#ifdef PARANOIA
	if (row < 0 || row + nrows > scr->nrows)
		return;
#endif

	bitmap = scr->view->bitmap;
	depth = bitmap->depth;
	bytesperrow = bitmap->bytes_per_row;
	rowmod = bitmap->row_mod;
	bmwidth = bytesperrow + rowmod;
	rowsize = bmwidth * scr->fontheight;
	bmapoffset = row * rowsize;

	if (rowmod == 0)
		fillsize = rowsize * nrows;
	else
		fillsize = 0;

	bgcolor = ATTRBG(attr);
	bgcolor = scr->colormap[bgcolor];

	for (j = 0 ; j < nrows ; j++)
		scr->rowmasks[row+j] = bgcolor;

	for (plane = 0 ; plane < depth ; plane++) {
		dst = bitmap->plane[plane] + bmapoffset;
		fill = (bgcolor & 1) ? 255 : 0; 

		if (fillsize > 0) {
			/* If the rows are continuous, write them all. */
			memset(dst, fill, fillsize);
		} else {
			for (j = 0 ; j < scr->fontheight * nrows ; j++) {
				memset(dst, fill, bytesperrow);
				dst += bmwidth;
			}
		}
		bgcolor >>= 1;
	}
}

int
amidisplaycc_alloc_attr(screen, fg, bg, flags, attrp)
	void  *screen;
	int    fg;
	int    bg;
	int    flags;
	long  *attrp;
{
	adccscr_t *scr;
	int maxcolor;
	int newfg;
	int newbg;

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
amidisplaycc_ioctl(dp, cmd, data, flag, p)
	void         *dp;
	u_long        cmd;
	caddr_t       data;
	int           flag;
	struct proc  *p; 
{
	switch (cmd)
	{
	case WSDISPLAYIO_GTYPE:
		*(u_int*)data = WSDISPLAY_TYPE_EGA; /* XXX */
		return (0);
	}

	printf("amidisplaycc_ioctl %lx (grp:'%c' num:%d)\n",
	       (long)cmd,
	       (char)((cmd&0xff00)>>8),
	       (int)(cmd&0xff));

	/* Yes, think should return -1 if didnt understand. */
	return (-1);
}

paddr_t
amidisplaycc_mmap(dp, off, prot)
	void   *dp;
	off_t   off;
	int     prot;
{
	return (-1);
}

int
amidisplaycc_alloc_screen(dp, screenp, cookiep, curxp, curyp, defattrp)
	void   *dp;
	const struct wsscreen_descr *screenp;
	void  **cookiep;
	int    *curxp;
	int    *curyp;
	long   *defattrp;
{
	dimen_t dimension;
	adccscr_t *scr;
	view_t *view;
	struct amidisplaycc_screen_descr *adccscreenp;
	int depth;
	int maxcolor;
	int i;
	int j;

	adccscreenp = (struct amidisplaycc_screen_descr*)screenp;
	depth = adccscreenp->depth;

	maxcolor = (1 << depth) - 1;

	/* Sanity checks because of fixed buffers */
	if (depth > MAXDEPTH || maxcolor >= MAXCOLORS)
		return (ENOMEM);
	if (screenp->nrows > MAXROWS)
		return (ENOMEM);

	dimension.width = screenp->ncols * 8;
	dimension.height = screenp->nrows * 8;

	view = grf_alloc_view(NULL, &dimension, depth);
	if (view == NULL)
		return (ENOMEM);

	/* 
	 * First screen gets the statically allocated console screen.
	 * Others are allocated dynamically.
	 */
	if (amidisplaycc_consolescreen.isconsole == 0) {
		scr = &amidisplaycc_consolescreen;
		scr->isconsole = 1;
	} else {
		scr = malloc(sizeof(adccscr_t), M_DEVBUF, M_WAITOK);
		bzero(scr, sizeof(adccscr_t));
	}

	scr->view = view;
	scr->fontheight = 8;

	scr->ncols = screenp->ncols;
	scr->nrows = screenp->nrows;

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
	 * This of course should be reflected on the palette
	 * but currently we don't do any palette management.
	 */
	for (i = 2 ; i < MAXCOLORS ; i *= 2) {
		j = i * 2 - 1;

		if (j < MAXCOLORS) {
			scr->colormap[i] = j;
			scr->colormap[j] = i;
		}
	}

	*cookiep = scr;

	*curxp = 0;
	*curyp = 0;
	amidisplaycc_cursor(scr, 1, *curxp, *curyp);

	*defattrp = MAKEATTR(maxcolor, 0, 0);

	/* Show the console automatically */
	if (scr->isconsole)
		grf_display_view(scr->view);

	return (0);
}

void
amidisplaycc_free_screen(dp, screen)
	void  *dp;
	void  *screen;
{
	adccscr_t *scr;
	
	scr = screen;

	if (scr == NULL)
		return;

	if (scr->view)
		grf_free_view(scr->view);
	scr->view = NULL;

	/* Take care not to free the statically allocated console screen */
	if (scr != &amidisplaycc_consolescreen) {
		free(scr, M_DEVBUF);
	}
}

int
amidisplaycc_show_screen(dp, screen, waitok, cb, cbarg)
	void  *dp;
	void  *screen;
	int    waitok;
	void (*cb) (void *, int, int);
	void  *cbarg;
{
	adccscr_t *scr;

	scr = screen;
	grf_display_view(scr->view);
    
	return (0);
}

/*
 * Load a font. Not supported yet.
 */
int
amidisplaycc_load_font(dp, cookie, fontp)
	void  *dp;
	void  *cookie;
	struct wsdisplay_font  *fontp;
{
	return (-1);
}

/* 
 * These dummy functions are here just so that we can compete of
 * the console at init.
 * If we win the console then the wscons system will provide the
 * real ones which in turn will call the apropriate wskbd device.
 * These should never be called.
 */

void
amidisplaycc_cnputc(cd,ch)
	dev_t cd;
	int ch;
{
}

int
amidisplaycc_cngetc(cd)
	dev_t cd;
{
	return (0);
}

void 
amidisplaycc_cnpollc(cd,on)
	dev_t cd;
	int on;
{
}

#endif /* AMIDISPLAYCC */




