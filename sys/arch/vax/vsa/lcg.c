/*	$NetBSD: lcg.c,v 1.3 2018/02/08 09:05:18 dholland Exp $ */
/*
 * LCG accelerated framebuffer driver
 * Copyright (c) 2003, 2004 Blaz Antonic
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the abovementioned copyrights
 * 4. The name of the author may not be used to endorse or promote products
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
 * Resurrection and dumb framebuffer mode by Björn Johannesson
 * rherdware@yahoo.com in December 2014
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lcg.c,v 1.3 2018/02/08 09:05:18 dholland Exp $");

#define LCG_NO_ACCEL

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/vsbus.h>
#include <machine/sid.h>
#include <machine/cpu.h>
#include <machine/lcgreg.h>

#include <dev/cons.h>

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>
#include <dev/dec/dzkbdvar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/wsfont/wsfont.h>

#include "machine/scb.h"

#include "dzkbd.h"

/* Screen hardware defs */

#define	LCG_FB_ADDR		0x21801000	/* Frame buffer */

/* FIFO defines */
#define LCG_FIFO_SIZE		0x10000	/* 64 KB */
#define LCG_FIFO_WIN_ADDR	0x20180000
#define LCG_FIFO_WIN_SIZE	VAX_NBPG
#define LCG_FIFO_ALIGN		0x10000

/* font rendering defines */
#define LCG_FONT_ADDR		(LCG_FB_ADDR + lcg_fb_size)
#define LCG_FONT_STORAGE_SIZE	0x40000	/* 16 KB, enough to accommodate 16x32 font bitmaps */

/* register space defines */
#define LCG_REG_ADDR	0x20100000	/* LCG registers */
#define LCG_REG_SIZE	0x4000		/* 16384 bytes */
#define LCG_REG(reg) regaddr[(reg / 4)]

/* LUT defines */
#define LCG_LUT_ADDR	0x21800800	/* LUT right before onscreen FB */
#define LCG_LUT_OFFSET	0x00000800
#define LCG_LUT_SIZE	0x800		/* 2048 bytes */

#define	LCG_BG_COLOR	WSCOL_BLACK
#define	LCG_FG_COLOR	WSCOL_WHITE

#define	LCG_CONFIG	0x200f0010	/* LCG model information */

/* implement sanity checks at certain points to ensure safer operation */
#define LCG_SAFE
//#define LCG_DEBUG

static	int lcg_match(struct device *, struct cfdata *, void *);
static	void lcg_attach(struct device *, struct device *, void *);

struct	lcg_softc {
	struct	device ss_dev;
	bus_dmamap_t sc_dm;
};

CFATTACH_DECL_NEW(lcg, sizeof(struct lcg_softc),
    lcg_match, lcg_attach, NULL, NULL);

static void	lcg_cursor(void *, int, int, int);
static int	lcg_mapchar(void *, int, unsigned int *);
static void	lcg_putchar(void *, int, int, u_int, long);
static void	lcg_copycols(void *, int, int, int,int);
static void	lcg_erasecols(void *, int, int, int, long);
static void	lcg_copyrows(void *, int, int, int);
static void	lcg_eraserows(void *, int, int, long);
static int	lcg_allocattr(void *, int, int, int, long *);
static int	lcg_get_cmap(struct wsdisplay_cmap *);
static int	lcg_set_cmap(struct wsdisplay_cmap *);
static void	lcg_init_common(struct device *, struct vsbus_attach_args *);

const struct wsdisplay_emulops lcg_emulops = {
	lcg_cursor,
	lcg_mapchar,
	lcg_putchar,
	lcg_copycols,
	lcg_erasecols,
	lcg_copyrows,
	lcg_eraserows,
	lcg_allocattr
};

static char lcg_stdscreen_name[10] = "160x68";
struct wsscreen_descr lcg_stdscreen = {
	lcg_stdscreen_name, 160, 68,		/* dynamically set */
	&lcg_emulops,
	8, 15,					/* dynamically set */
	WSSCREEN_UNDERLINE|WSSCREEN_REVERSE|WSSCREEN_WSCOLORS,
};

const struct wsscreen_descr *_lcg_scrlist[] = {
	&lcg_stdscreen,
};

const struct wsscreen_list lcg_screenlist = {
	sizeof(_lcg_scrlist) / sizeof(struct wsscreen_descr *),
	_lcg_scrlist,
};

static	char *lcgaddr;
static	char *lutaddr;
static	volatile long *regaddr;
static	volatile long *fifoaddr;
#ifndef LCG_NO_ACCEL
static	char *fontaddr;
#endif

static int	lcg_xsize;
static int	lcg_ysize;
static int	lcg_depth;
static int	lcg_cols;
static int	lcg_rows;
static int	lcg_onerow;
static int	lcg_fb_size;
static int	lcg_glyph_size; /* bitmap size in bits */

static	char *cursor;

static	int cur_on;

static	int cur_fg, cur_bg;


/* Our current hardware colormap */
static struct hwcmap256 {
#define	CMAP_SIZE	256	/* 256 R/G/B entries */
	u_int8_t r[CMAP_SIZE];
	u_int8_t g[CMAP_SIZE];
	u_int8_t b[CMAP_SIZE];
} lcg_cmap;

/* The default colormap */
static struct {
	u_int8_t r[8];
	u_int8_t g[8];
	u_int8_t b[8];
} lcg_default_cmap = {
	{ 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff },
	{ 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0xff },
	{ 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff }
};

struct wsdisplay_font lcg_font;
static u_char *qf;
static u_short *qf2;

#define QCHAR(c) (c < lcg_font.firstchar ? 0 : \
	(c >= (lcg_font.firstchar + lcg_font.numchars) ? 0 : c - lcg_font.firstchar))
#define QFONT(c,line)	((lcg_font.stride == 2 ? \
	qf2[QCHAR(c) * lcg_font.fontheight + line] : \
	qf[QCHAR(c) * lcg_font.fontheight + line]))
#define	LCG_ADDR(row, col, line, dot) \
	lcgaddr[((col) * lcg_font.fontwidth) + ((row) * lcg_font.fontheight * lcg_xsize) + \
	    (line) * lcg_xsize + dot]


static int	lcg_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	lcg_mmap(void *, void *, off_t, int);
static int	lcg_alloc_screen(void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *);
static void	lcg_free_screen(void *, void *);
static int	lcg_show_screen(void *, void *, int,
				     void (*) (void *, int, int), void *);
static void	lcg_crsr_blink(void *);

/* LCG HW accel functions */
#ifndef LCG_NO_ACCEL
static void	fifo_put(long data);
static int	fifo_fill(int iterations);
static u_char	fifo_counter = 0;

static void	blkcpy(long source, long dest, int xdim, int ydim);
static void	blkset(long dest, int xdim, int ydim, char color);
static void	renderchar(long source, long dest, int xdim, int ydim, char fg, char bg);
#endif /* LCG_NO_ACCEL */

const struct wsdisplay_accessops lcg_accessops = {
	lcg_ioctl,
	lcg_mmap,
	lcg_alloc_screen,
	lcg_free_screen,
	lcg_show_screen,
	0 /* load_font */
};

/* TODO allocate ss_image dynamically for consoles beyond first one */
struct	lcg_screen {
	int	ss_curx;
	int	ss_cury;
	int	ss_cur_fg;
	int	ss_cur_bg;
	struct {
		u_char	data;			/* Image character */
		u_char	attr;			/* Attribute: 80/70/08/07 */
	} ss_image[160 * 128];			/* allow for maximum possible cell matrix */
};
#define	LCG_ATTR_UNDERLINE	0x80
#define	LCG_BG_MASK		0x70
#define	LCG_ATTR_REVERSE	0x08
#define	LCG_FG_MASK		0x07

static	struct lcg_screen lcg_conscreen;
static	struct lcg_screen *curscr;
static	struct lcg_screen *savescr;

static	callout_t lcg_cursor_ch;

#ifndef LCG_NO_ACCEL
void fifo_put(long data)
{
	fifo_counter &= 0x3;
	fifoaddr[fifo_counter] = data;
	fifo_counter++;
}

int fifo_fill(int iterations)
{
	long status;
	int counter = 0;;

	while (fifo_counter % 4)
		fifo_put(0);

#ifdef LCG_SAFE
	status = LCG_REG(LCG_REG_GRAPHICS_SUB_STATUS);
	while ((counter < iterations) && ((status & 0x80000000) == 0x80000000)) {
		delay(1000);
		status = LCG_REG(LCG_REG_GRAPHICS_SUB_STATUS);
		counter++;
	}
#endif

	if (counter == 0)
		return 0;
	else
		return 1;
}

void blkcpy(long source, long dest, int xdim, int ydim)
{
	int err;

#ifdef LCG_SAFE
	if ((source < LCG_FB_ADDR) || (source > LCG_FB_ADDR + lcg_fb_size)) {
		printf("lcg: blkcpy: invalid source 0x%lx\n", source);
		return;
	}
	if ((dest < LCG_FB_ADDR) || (dest > LCG_FB_ADDR + lcg_fb_size)) {
		printf("lcg: blkcpy: invalid destination 0x%lx\n", dest);
		return;
	}
#endif

#ifdef LCG_SAFE
	fifo_put(0x0c010000 | (cur_fg & 0xff));
#endif
	fifo_put(0x01020006);

	fifo_put(0x06800000);
	fifo_put(source);
	fifo_put(lcg_xsize);

	fifo_put(0x05800000);
	fifo_put(dest);
	fifo_put(lcg_xsize);

	fifo_put(0x03400000);
	fifo_put(0xff);

	fifo_put(0x02000003);

#ifdef LCG_SAFE
	fifo_put(0x04c00000);
	fifo_put(0);
	fifo_put(lcg_xsize);
	fifo_put(lcg_fb_size - (dest - LCG_FB_ADDR));
#endif

	fifo_put(0x09c00000);
	fifo_put(((ydim & 0xffff) << 16) | xdim);
	fifo_put(0);
	fifo_put(0);

	err = fifo_fill(200);
	if (err)
		printf("lcg: AG still busy after 200 msec\n");
}

void blkset(long dest, int xdim, int ydim, char color)
{
	int err;

#ifdef LCG_SAFE
	if ((dest < LCG_FB_ADDR) || (dest > LCG_FB_ADDR + lcg_fb_size)) {
		printf("lcg: blkset: invalid destination 0x%lx\n", dest);
		return;
	}
#endif

	fifo_put(0x0c010000 | (color & 0xff));

	fifo_put(0x01000000);

	fifo_put(0x05800000);
	fifo_put(dest);
	fifo_put(lcg_xsize);

	fifo_put(0x03400000);
	fifo_put(0xff);

	fifo_put(0x02000003);

#ifdef LCG_SAFE
	fifo_put(0x04c00000);
	fifo_put(0);
	fifo_put(lcg_xsize);
	fifo_put(lcg_fb_size - (dest - LCG_FB_ADDR));
#endif

	fifo_put(0x09c00000);
	fifo_put(((ydim & 0xffff) << 16) | xdim);
	fifo_put(0);
	fifo_put(0);

	err = fifo_fill(200);
	if (err)
		printf("lcg: AG still busy after 200 msec\n");
}

void renderchar(long source, long dest, int xdim, int ydim, char fg, char bg)
{
	int err;
#ifdef LCG_SAFE
	if ((dest < LCG_FB_ADDR) || (dest > LCG_FB_ADDR + lcg_fb_size)) {
		printf("lcg: blkset: invalid destination 0x%lx\n", dest);
		return;
	}
#endif

	fifo_put(0x0c050000 | (bg & 0xff));
	fifo_put(0x0c010000 | (fg & 0xff));

	fifo_put(0x01030008);

	fifo_put(0x06800000);
	fifo_put(source);
	//fifo_put(lcg_xsize);
	fifo_put(lcg_font.stride);

	fifo_put(0x05800000);
	fifo_put(dest);
	fifo_put(lcg_xsize);

	fifo_put(0x03400000);
	fifo_put(0xff);

	fifo_put(0x02000003);

#ifdef LCG_SAFE
	fifo_put(0x04c00000);
	fifo_put(0);
	fifo_put(lcg_xsize);
	fifo_put(lcg_fb_size - (dest - LCG_FB_ADDR));
#endif

	fifo_put(0x09c00000);
	fifo_put(((ydim & 0xffff) << 16) | (xdim & 0xffff));
	fifo_put(0);
	fifo_put(0);

	err = fifo_fill(200);
	if (err)
		printf("lcg: AG still busy after 200 msec\n");
}
#endif /* LCG_NO_ACCEL */

int
lcg_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct vsbus_softc *sc = device_private(parent);
	struct vsbus_attach_args *va = aux;
	char *ch = (char *)va->va_addr;

	if ((vax_boardtype != VAX_BTYP_46) && (vax_boardtype != VAX_BTYP_48))
		return 0;

	*ch = 1;
	if ((*ch & 1) == 0)
		return 0;
	*ch = 0;
	if ((*ch & 1) != 0)
		return 0;

	/* XXX use vertical interrupt? */
	sc->sc_mask = 0x04; /* XXX - should be generated */
	scb_fake(0x120, 0x15);
	return 20;
}

void
lcg_attach(struct device *parent, struct device *self, void *aux)
{
	struct vsbus_attach_args *va = aux;
	struct wsemuldisplaydev_attach_args aa;

	printf("\n");
	aa.console = lcgaddr != NULL;

	lcg_init_common(self, va);

	curscr = &lcg_conscreen;

	aa.scrdata = &lcg_screenlist;
	aa.accessops = &lcg_accessops;

	/* enable software cursor */
	callout_init(&lcg_cursor_ch, 0);
	callout_reset(&lcg_cursor_ch, hz / 2, lcg_crsr_blink, NULL);

	config_found(self, &aa, wsemuldisplaydevprint);
}

static void
lcg_crsr_blink(void *arg)
{
	int dot;

	if (cur_on && curscr != NULL)
		for (dot = 0; dot < lcg_font.fontwidth; dot++)
			cursor[dot] = ((cursor[dot] & 0x0f) == cur_fg) ? cur_bg : cur_fg;

	callout_reset(&lcg_cursor_ch, hz / 2, lcg_crsr_blink, NULL);
}

void
lcg_cursor(void *id, int on, int row, int col)
{
	struct lcg_screen *ss = id;
	int dot, attr;

	attr = ss->ss_image[row * lcg_cols + col].attr;
	if (ss == curscr) {
		if (cursor != NULL) {
			int ch = QFONT(ss->ss_image[ss->ss_cury * lcg_cols +
			    ss->ss_curx].data, lcg_font.fontheight - 1);
			attr = ss->ss_image[ss->ss_cury * lcg_cols +
			    ss->ss_curx].attr;

			if (attr & LCG_ATTR_REVERSE) {
				cur_bg = attr & LCG_FG_MASK;
				cur_fg = (attr & LCG_BG_MASK) >> 4;
			} else {
				cur_fg = attr & LCG_FG_MASK;
				cur_bg = (attr & LCG_BG_MASK) >> 4;
			}
			for (dot = 0; dot < lcg_font.fontwidth; dot++)
				cursor[dot] =  (ch & (1 << dot)) ?
				    cur_fg : cur_bg;
		}

		cursor = &LCG_ADDR(row, col, lcg_font.fontheight - 1, 0);
		cur_on = on;
		if (attr & LCG_ATTR_REVERSE) {
			cur_bg = attr & LCG_FG_MASK;
			cur_fg = (attr & LCG_BG_MASK) >> 4;
		} else {
			cur_fg = attr & LCG_FG_MASK;
			cur_bg = (attr & LCG_BG_MASK) >> 4;
		}
	}
	ss->ss_curx = col;
	ss->ss_cury = row;
	if (attr & LCG_ATTR_REVERSE) {
		ss->ss_cur_bg = attr & LCG_FG_MASK;
		ss->ss_cur_fg = (attr & LCG_BG_MASK) >> 4;
	} else {
		ss->ss_cur_fg = attr & LCG_FG_MASK;
		ss->ss_cur_bg = (attr & LCG_BG_MASK) >> 4;
	}
}

int
lcg_mapchar(void *id, int uni, unsigned int *index)
{
	if (uni < 256) {
		*index = uni;
		return (5);
	}
	*index = ' ';
	return (0);
}

static void
lcg_putchar(void *id, int row, int col, u_int c, long attr)
{
	struct lcg_screen *ss = id;
	int i;
	char dot_fg, dot_bg;

	c &= 0xff;

	ss->ss_image[row * lcg_cols + col].data = c;
	ss->ss_image[row * lcg_cols + col].attr = attr;
	if (ss != curscr)
		return;

	dot_fg = attr & LCG_FG_MASK;
	dot_bg = (attr & LCG_BG_MASK) >> 4;
	if (attr & LCG_ATTR_REVERSE) {
		dot_fg = (attr & LCG_BG_MASK) >> 4;
		dot_bg = attr & LCG_FG_MASK;
	}

#ifndef LCG_NO_ACCEL
	renderchar(LCG_FONT_ADDR + (c * lcg_glyph_size),
		   LCG_FB_ADDR + (row * lcg_onerow) + (col * lcg_font.fontwidth),
		   lcg_font.fontwidth, lcg_font.fontheight, dot_fg, dot_bg);
#else
	for (i = 0; i < lcg_font.fontheight; i++) {
		unsigned char ch = QFONT(c,i);
		for (int j = 0; j < lcg_font.fontwidth; j++) {
			LCG_ADDR(row, col, i, j) = ((ch >> j) & 1) ? dot_fg : dot_bg;
		}
	}
#endif /* LCG_NO_ACCEL */
	if (attr & LCG_ATTR_UNDERLINE) {
		char *p = &LCG_ADDR(row, col, lcg_font.fontheight - 1, 0);
		for (i = 0; i < lcg_font.fontwidth; i++)
			p[i] = ~p[i];
	}
}



/*
 * copies columns inside a row.
 */
static void
lcg_copycols(void *id, int row, int srccol, int dstcol, int ncols)
{
	struct lcg_screen *ss = id;
#ifdef LCG_NO_ACCEL
	int i = 0;
#endif

	bcopy(&ss->ss_image[row * lcg_cols + srccol], &ss->ss_image[row *
	    lcg_cols + dstcol], ncols * sizeof(ss->ss_image[0]));
	if (ss != curscr)
		return;
#ifdef LCG_NO_ACCEL
	for (i = 0; i < lcg_font.fontheight; i++)
		memcpy(&LCG_ADDR(row, dstcol, i, 0),
		&LCG_ADDR(row,srccol, i, 0), ncols * lcg_font.fontwidth);

#else
	blkcpy(LCG_FB_ADDR + (row * lcg_onerow) + (srccol * lcg_font.fontwidth),
	       LCG_FB_ADDR + (row * lcg_onerow) + (dstcol * lcg_font.fontwidth),
	       (ncols * lcg_font.fontwidth), lcg_font.fontheight);
#endif
}

/*
 * Erases a bunch of chars inside one row.
 */
static void
lcg_erasecols(void *id, int row, int startcol, int ncols, long fillattr)
{
	struct lcg_screen *ss = id;
	int i;

	bzero(&ss->ss_image[row * lcg_cols + startcol], ncols * sizeof(ss->ss_image[0]));
	for (i = row * lcg_cols + startcol; i < row * lcg_cols + startcol + ncols; ++i)
		ss->ss_image[i].attr = fillattr;
	if (ss != curscr)
		return;
#ifdef LCG_NO_ACCEL
	for (i = 0; i < lcg_font.fontheight; i++)
                memset(&LCG_ADDR(row, startcol, i, 0), 0, ncols * lcg_font.fontwidth);
#else
	blkset(LCG_FB_ADDR + (row * lcg_onerow) + (startcol * lcg_font.fontwidth),
		(ncols * lcg_font.fontwidth), lcg_font.fontheight, (fillattr & LCG_BG_MASK) >> 4);
#endif
}

static void
lcg_copyrows(void *id, int srcrow, int dstrow, int nrows)
{
	struct lcg_screen *ss = id;

	bcopy(&ss->ss_image[srcrow * lcg_cols], &ss->ss_image[dstrow * lcg_cols],
	    nrows * lcg_cols * sizeof(ss->ss_image[0]));
	if (ss != curscr)
		return;
#ifdef LCG_NO_ACCEL
	memcpy(&lcgaddr[dstrow * lcg_onerow],
            &lcgaddr[srcrow * lcg_onerow], nrows * lcg_onerow);
#else
	blkcpy(LCG_FB_ADDR + (srcrow * lcg_onerow), LCG_FB_ADDR + (dstrow * lcg_onerow),
		(lcg_cols * lcg_font.fontwidth), (nrows * lcg_font.fontheight));
#endif
}

static void
lcg_eraserows(void *id, int startrow, int nrows, long fillattr)
{
	struct lcg_screen *ss = id;
	int i;

	bzero(&ss->ss_image[startrow * lcg_cols], nrows * lcg_cols *
	    sizeof(ss->ss_image[0]));
	for (i = startrow * lcg_cols; i < (startrow + nrows) * lcg_cols; ++i)
		ss->ss_image[i].attr = fillattr;
	if (ss != curscr)
		return;
#ifdef LCG_NO_ACCEL
	memset(&lcgaddr[startrow * lcg_onerow], 0, nrows * lcg_onerow);
#else
	blkset(LCG_FB_ADDR + (startrow * lcg_onerow), (lcg_cols * lcg_font.fontwidth), 
		(nrows * lcg_font.fontheight), (fillattr & LCG_BG_MASK) >> 4);
#endif
}

static int
lcg_allocattr(void *id, int fg, int bg, int flags, long *attrp)
{
	long myattr;

	if (flags & WSATTR_WSCOLORS)
		myattr = (fg & LCG_FG_MASK) | ((bg << 4) & LCG_BG_MASK);
	else
		myattr = WSCOL_WHITE << 4;	/* XXXX */
	if (flags & WSATTR_REVERSE)
		myattr |= LCG_ATTR_REVERSE;
	if (flags & WSATTR_UNDERLINE)
		myattr |= LCG_ATTR_UNDERLINE;
	*attrp = myattr;
	return 0;
}

static int
lcg_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct wsdisplay_fbinfo *fb = (void *)data;
	int i;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_LCG;
		break;

	case WSDISPLAYIO_GINFO:
		fb->height = lcg_ysize;
		fb->width = lcg_xsize;
		fb->depth = lcg_depth;
		fb->cmsize = 1 << lcg_depth;
		break;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = lcg_xsize;
		break;

	case WSDISPLAYIO_GETCMAP:
		return lcg_get_cmap((struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return lcg_set_cmap((struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_GMODE:
		return EPASSTHROUGH;

	case WSDISPLAYIO_SMODE:
		/* if setting WSDISPLAYIO_MODE_EMUL, restore console cmap, current screen */
		if (*(u_int *)data == WSDISPLAYIO_MODE_EMUL) {
			/* FIXME: if this is meant to reset palette LUT reload has to be triggered too */
			bzero(lutaddr, LCG_LUT_SIZE);
			for (i = 0; i < 8; ++i) {
				lcg_cmap.r[i] = lcg_default_cmap.r[i];
				lcg_cmap.g[i] = lcg_default_cmap.g[i];
				lcg_cmap.b[i] = lcg_default_cmap.b[i];
				lutaddr[i * 8 + 1] = i;
				lutaddr[i * 8 + 2] = 1;
				lutaddr[i * 8 + 3] = lcg_cmap.r[i] >> (lcg_depth & 7);
				lutaddr[i * 8 + 4] = 1;
				lutaddr[i * 8 + 5] = lcg_cmap.g[i] >> (lcg_depth & 7);
				lutaddr[i * 8 + 6] = 1;
				lutaddr[i * 8 + 7] = lcg_cmap.b[i] >> (lcg_depth & 7);
			}
			if (savescr != NULL)
				lcg_show_screen(NULL, savescr, 0, NULL, NULL);
			savescr = NULL;
		} else {		/* WSDISPLAYIO_MODE_MAPPED */
			savescr = curscr;
			curscr = NULL;
			/* clear screen? */
		}

		return EPASSTHROUGH;
#if 0
	case WSDISPLAYIO_SVIDEO:
		if (*(u_int *)data == WSDISPLAYIO_VIDEO_ON) {
			curcmd = curc;
		} else {
			curc = curcmd;
			curcmd &= ~(CUR_CMD_FOPA|CUR_CMD_ENPA);
			curcmd |= CUR_CMD_FOPB;
		}
		WRITECUR(CUR_CMD, curcmd);
		break;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = (curcmd & CUR_CMD_FOPB ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON);
		break;

#endif
	default:
		return EPASSTHROUGH;
	}
	return 0;
}

static paddr_t
lcg_mmap(void *v, void *vs, off_t offset, int prot)
{
	if (offset >= lcg_fb_size || offset < 0)
		return -1;
	return (LCG_FB_ADDR + offset) >> PGSHIFT;
}

int
lcg_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *defattrp)
{
	int i;
	struct lcg_screen *ss;

	*cookiep = malloc(sizeof(struct lcg_screen), M_DEVBUF, M_WAITOK);
	bzero(*cookiep, sizeof(struct lcg_screen));
	*curxp = *curyp = 0;
	*defattrp = (LCG_BG_COLOR << 4) | LCG_FG_COLOR;
	ss = *cookiep;
	for (i = 0; i < lcg_cols * lcg_rows; ++i)
		ss->ss_image[i].attr = (LCG_BG_COLOR << 4) | LCG_FG_COLOR;
	return 0;
}

void
lcg_free_screen(void *v, void *cookie)
{
}

int
lcg_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	struct lcg_screen *ss = cookie;
	int row, col, dot_fg, dot_bg, j, attr;
#ifdef LCG_NO_ACCEL
	int iter, jter;
	unsigned char ch;
#endif

	if (ss == curscr)
		return (0);
#ifdef LCG_NO_ACCEL
	memset(lcgaddr, LCG_BG_COLOR, lcg_xsize * lcg_ysize);
#endif

	for (row = 0; row < lcg_rows; row++)
		for (col = 0; col < lcg_cols; col++) {
			attr = ss->ss_image[row * lcg_cols + col].attr;
			if (attr & LCG_ATTR_REVERSE) {
				dot_fg = (attr & LCG_BG_MASK) >> 4;
				dot_bg = attr & LCG_FG_MASK;
			} else {
				dot_fg = attr & LCG_FG_MASK;
				dot_bg = (attr & LCG_BG_MASK) >> 4;
			}
			u_char c = ss->ss_image[row * lcg_cols + col].data;

			if (c < 32)
				c = 32;
#ifndef LCG_NO_ACCEL
			renderchar(LCG_FONT_ADDR + (c * lcg_glyph_size),
				   LCG_FB_ADDR + (row * lcg_onerow) + (col * lcg_font.fontwidth),
				   lcg_font.fontwidth, lcg_font.fontheight, dot_fg, dot_bg);
#else
			for (iter = 0; iter < lcg_font.fontheight; iter++) {
				ch = QFONT(c,iter);
				for (jter = 0; jter < lcg_font.fontwidth; jter++) {
					LCG_ADDR(row, col, iter, jter) = ((ch >> jter) & 1) ? dot_fg : dot_bg;
				}
			}
#endif
			if (attr & LCG_ATTR_UNDERLINE)
				for (j = 0; j < lcg_font.fontwidth; j++)
					LCG_ADDR(row, col,
					    lcg_font.fontheight - 1, j) = dot_fg;
		}

	cursor = &lcgaddr[(ss->ss_cury * lcg_onerow) +
		((lcg_font.fontheight - 1) * lcg_xsize) +
			(ss->ss_curx * lcg_font.fontwidth)];
	cur_fg = ss->ss_cur_fg;
	cur_bg = ss->ss_cur_bg;

	curscr = ss;
	return (0);
}

static int
lcg_get_cmap(struct wsdisplay_cmap *p)
{
	u_int index = p->index, count = p->count;
	int error;

	if (index >= (1 << lcg_depth) || count > (1 << lcg_depth) - index)
		return (EINVAL);

	error = copyout(&lcg_cmap.r[index], p->red, count);
	if (error)
		return error;
	error = copyout(&lcg_cmap.g[index], p->green, count);
	if (error)
		return error;
	error = copyout(&lcg_cmap.b[index], p->blue, count);
	return error;
}

static int
lcg_set_cmap(struct wsdisplay_cmap *p)
{
	u_int index = p->index, count = p->count;
	int error, s;

	if (index >= (1 << lcg_depth) || count > (1 << lcg_depth) - index)
		return (EINVAL);

	error = copyin(p->red, &lcg_cmap.r[index], count);
	if (error)
		return error;
	error = copyin(p->green, &lcg_cmap.g[index], count);
	if (error)
		return error;
	error = copyin(p->blue, &lcg_cmap.b[index], count);
	if (error)
		return error;

	s = spltty();
	/* FIXME: if this is meant to set palette LUT reload has to be triggered too */
	while (count-- > 0) {
		lutaddr[index * 8 + 0] = 0;
		lutaddr[index * 8 + 1] = index;
		lutaddr[index * 8 + 2] = 1;
		lutaddr[index * 8 + 3] = lcg_cmap.r[index] >> (lcg_depth & 7);
		lutaddr[index * 8 + 4] = 1;
		lutaddr[index * 8 + 5] = lcg_cmap.g[index] >> (lcg_depth & 7);
		lutaddr[index * 8 + 6] = 1;
		lutaddr[index * 8 + 7] = lcg_cmap.b[index] >> (lcg_depth & 7);
		++index;
	}
	splx(s);
	return (0);
}

cons_decl(lcg);

void
lcgcninit(struct consdev *cndev)
{
	int i;
	/* Clear screen */
	memset(lcgaddr, LCG_BG_COLOR, lcg_xsize * lcg_ysize);

	curscr = &lcg_conscreen;
	for (i = 0; i < lcg_cols * lcg_rows; ++i)
		lcg_conscreen.ss_image[i].attr =
		    (LCG_BG_COLOR << 4) | LCG_FG_COLOR;
	wsdisplay_cnattach(&lcg_stdscreen, &lcg_conscreen, 0, 0,
	    (LCG_BG_COLOR << 4) | LCG_FG_COLOR);
	cn_tab->cn_pri = CN_INTERNAL;


#if NDZKBD > 0
	dzkbd_cnattach(0); /* Connect keyboard and screen together */
#endif
}

/*
 * Called very early to setup the glass tty as console.
 * Because it's called before the VM system is inited, virtual memory
 * for the framebuffer can be stolen directly without disturbing anything.
 */
void
lcgcnprobe(struct consdev *cndev)
{
	extern const struct cdevsw wsdisplay_cdevsw;

	if ((vax_boardtype != VAX_BTYP_46) && (vax_boardtype != VAX_BTYP_48))
		return; /* Only for VS 4000/60 and VLC */

	if (vax_confdata & 0x100)
		return; /* Diagnostic console */

	lcg_init_common(NULL, NULL);

	/* Set up default LUT */

	cndev->cn_pri = CN_INTERNAL;
	cndev->cn_dev = makedev(cdevsw_lookup_major(&wsdisplay_cdevsw), 0);
}

void
lcg_init_common(struct device *self, struct vsbus_attach_args *va)
{
	u_int magic, *lcg_config;
	int i;
	extern vaddr_t virtual_avail;
	long video_conf;
	int cookie;
	struct wsdisplay_font *wf;

	struct lcg_softc *sc = (void *)self;
	bus_dma_segment_t seg;
	int rseg, err;
	void *fifo_mem_vaddr;
#ifndef LCG_NO_ACCEL
	u_char line;
	u_int ch, temp;
#endif

	if (regaddr != NULL)
		return;

	/* map LCG registers first */
	if (self != NULL) {
		regaddr = (long*)vax_map_physmem(LCG_REG_ADDR, (LCG_REG_SIZE/VAX_NBPG));
		if (regaddr == 0) {
			printf("%s: Couldn't allocate register memory.\n", self->dv_xname);
			return;
		}
	} else {
		regaddr = (long*)virtual_avail;
		virtual_avail += LCG_REG_SIZE;
		ioaccess((vaddr_t)regaddr, LCG_REG_ADDR, (LCG_REG_SIZE/VAX_NBPG));
	}

	/*
	 * v = *0x200f0010 & VLC ? 0x07 : 0xf0;
	 * switch(v) {
	 * 0x05: 1280x1024
	 * 0x06: conf & 0x80 ? 1024x768 : 640x480
	 * 0x07: conf & 0x80 ? 1024x768 ? 1024x864
	 * 0x20: 1024x864
	 * 0x40: 1024x768
	 * 0x60: 1024x864
	 * 0x80: 1280x1024 4BPN
	 * 0x90: 1280x1024 8BPN
	 * 0xb0: 1280x1024 8BPN 2HD
	 */
	if (self != NULL) {
		lcg_config = (u_int *)vax_map_physmem(LCG_CONFIG, 1);
	} else {
		lcg_config = (u_int *)virtual_avail;
		ioaccess((vaddr_t)lcg_config, LCG_CONFIG, 1);
	}
	magic = *lcg_config & (vax_boardtype == VAX_BTYP_46 ? 0xf0 : 0x07);
	if (self != NULL) {
		vax_unmap_physmem((vaddr_t)lcg_config, 1);
	} else {
		iounaccess((vaddr_t)lcg_config, 1);
	}
	lcg_depth = 8;
	switch(magic) {
	case 0x80:		/* KA46 HR 1280x1024 4BPLN */
		lcg_depth = 4;
	case 0x05:		/* KA48 HR 1280x1024 8BPLN */
	case 0x90:		/* KA46 HR 1280x1024 8BPLN */
	case 0xb0:		/* KA46 HR 1280x1024 8BPLN 2HD */
		lcg_xsize = 1280;
		lcg_ysize = 1024;
		break;
	case 0x06:		/* KA48 1024x768 or 640x480 */
		if (vax_confdata & 0x80) {
			lcg_xsize = 1024;
			lcg_ysize = 768;
		} else {
			lcg_xsize = 640;
			lcg_ysize = 480;
		}
		break;
	case 0x07:		/* KA48 1024x768 or 1024x864 */
		lcg_xsize = 1024;
		lcg_ysize = (vax_confdata & 0x80) ? 768 : 864;
		break;
	case 0x20:		/* KA46 1024x864 */
	case 0x60:		/* KA46 1024x864 */
		lcg_xsize = 1024;
		lcg_ysize = 864;
		break;
	case 0x40:		/* KA46 1024x768 */
		lcg_xsize = 1024;
		lcg_ysize = 768;
		break;
	default:
		panic("LCG model not supported");
	}
	if (self != NULL)
		aprint_normal("%s: framebuffer size %dx%d, depth %d (magic 0x%x)\n",
			self->dv_xname, lcg_xsize, lcg_ysize, lcg_depth, magic);

	wsfont_init();
	cookie = wsfont_find(NULL, 12, 22, 0, WSDISPLAY_FONTORDER_R2L,
	    WSDISPLAY_FONTORDER_L2R, WSFONT_FIND_BITMAP);
	if (cookie == -1)
		cookie = wsfont_find(NULL, 8, 0, 0, WSDISPLAY_FONTORDER_R2L, 0,
				WSFONT_FIND_BITMAP);
	if (cookie == -1)
		cookie = wsfont_find(NULL, 0, 0, 0, WSDISPLAY_FONTORDER_R2L,
		    WSDISPLAY_FONTORDER_L2R, WSFONT_FIND_BITMAP);
	if (cookie == -1 || wsfont_lock(cookie, &wf))
		panic("lcg_common_init: can't load console font");
	lcg_font = *wf;
	lcg_cols = lcg_xsize / lcg_font.fontwidth;
	lcg_rows = lcg_ysize / lcg_font.fontheight;
	if (self != NULL) {
		aprint_normal("%s: using font %s (%dx%d), ", self->dv_xname, lcg_font.name,
				lcg_font.fontwidth, lcg_font.fontheight);
		aprint_normal("console size: %dx%d\n", lcg_cols, lcg_rows);
	}
	lcg_onerow = lcg_xsize * lcg_font.fontheight;
	lcg_fb_size = lcg_xsize * lcg_ysize;
	lcg_stdscreen.ncols = lcg_cols;
	lcg_stdscreen.nrows = lcg_rows;
	lcg_stdscreen.fontwidth = lcg_font.fontwidth;
	lcg_stdscreen.fontheight = lcg_font.fontheight;
	lcg_glyph_size = lcg_font.stride * lcg_font.fontheight;
	snprintf(lcg_stdscreen_name, sizeof(lcg_stdscreen_name), "%dx%d", lcg_cols, lcg_rows);
	qf = lcg_font.data;
	qf2 = (u_short *)lcg_font.data;

	if (self != NULL) {
		lcgaddr = (void *)vax_map_physmem(va->va_paddr,
					((lcg_fb_size + LCG_FONT_STORAGE_SIZE)/VAX_NBPG));
		if (lcgaddr == 0) {
			printf("%s: unable to allocate framebuffer memory.\n", self->dv_xname);
			return;
		}
#ifndef LCG_NO_ACCEL
		fontaddr = lcgaddr + lcg_fb_size;

		/* copy font bitmaps */
		for (ch = 0; ch < 256; ch++)
			for (line = 0; line < lcg_font.fontheight; line++) {
				temp = QFONT(ch, line);
				if (lcg_font.stride == 1)
					fontaddr[(ch * lcg_font.fontheight) + line] = temp;
				else { 
					/* stride == 2 */
					fontaddr[(ch * lcg_font.stride * lcg_font.fontheight) + line] = temp & 0xff;
					fontaddr[(ch * lcg_font.stride * lcg_font.fontheight) + line + 1] = (temp >> 16) & 0xff;
				}
			}
#endif

		lutaddr = (void *)vax_map_physmem(LCG_LUT_ADDR, (LCG_LUT_SIZE/VAX_NBPG));
		if (lutaddr == 0) {
			printf("%s: unable to allocate LUT memory.\n", self->dv_xname);
			return;
		}
		fifoaddr = (long*)vax_map_physmem(LCG_FIFO_WIN_ADDR, (LCG_FIFO_WIN_SIZE/VAX_NBPG));
		if (regaddr == 0) {
			printf("%s: unable to map FIFO window\n", self->dv_xname);
			return;
		}

		/* allocate contiguous physical memory block for FIFO */
		err = bus_dmamem_alloc(va->va_dmat, LCG_FIFO_SIZE, 
			LCG_FIFO_ALIGN, 0, &seg, 1, &rseg, BUS_DMA_NOWAIT);
		if (err) {
			printf("%s: unable to allocate FIFO memory block, err = %d\n", 
				self->dv_xname, err);
			return;
		}

		err = bus_dmamem_map(va->va_dmat, &seg, rseg, LCG_FIFO_SIZE,
			&fifo_mem_vaddr, BUS_DMA_NOWAIT);
		if (err) {
			printf("%s: unable to map FIFO memory block, err = %d\n", 
				self->dv_xname, err);
			bus_dmamem_free(va->va_dmat, &seg, rseg);
			return;
		}

		err = bus_dmamap_create(va->va_dmat, LCG_FIFO_SIZE, rseg,
			LCG_FIFO_SIZE, 0, BUS_DMA_NOWAIT, &sc->sc_dm);
		if (err) {
			printf("%s: unable to create DMA map, err = %d\n", self->dv_xname, err);
			bus_dmamem_unmap(va->va_dmat, fifo_mem_vaddr, LCG_FIFO_SIZE);
			bus_dmamem_free(va->va_dmat, &seg, rseg);
			return;
		}

		err = bus_dmamap_load(va->va_dmat, sc->sc_dm, fifo_mem_vaddr, 
			LCG_FIFO_SIZE, NULL, BUS_DMA_NOWAIT);
		if (err) {
			printf("%s: unable to load DMA map, err = %d\n", self->dv_xname, err);
			bus_dmamap_destroy(va->va_dmat, sc->sc_dm);
			bus_dmamem_unmap(va->va_dmat, fifo_mem_vaddr, LCG_FIFO_SIZE);
			bus_dmamem_free(va->va_dmat, &seg, rseg);
			return;
		}

		/* initialize LCG hardware */
		LCG_REG(LCG_REG_GRAPHICS_CONTROL) = 0xd8000000; /* gfx reset, FIFO and AG enable */
//		LCG_REG(LCG_REG_WRITE_PROTECT_LOW_HIGH) = (((LCG_FB_ADDR + lcg_fb_size) & 0xffff0000) | (LCG_FB_ADDR >> 16));
		LCG_REG(LCG_REG_WRITE_PROTECT_LOW_HIGH) = 0xffff0000;
		LCG_REG(LCG_REG_FIFO_BASE) = sc->sc_dm->dm_segs[0].ds_addr;
		LCG_REG(LCG_REG_FIFO_BASE2) = sc->sc_dm->dm_segs[0].ds_addr;
		LCG_REG(LCG_REG_FIFO_MASKS) = 0xffff;
		LCG_REG(LCG_REG_FIFO_SAVE_HEAD_OFFSET) = sc->sc_dm->dm_segs[0].ds_addr;
//		LCG_REG(LCG_REG_GRAPHICS_INT_STATUS) = 0;
//		LCG_REG(LCG_REG_GRAPHICS_CONTROL) = 0x50000000; /* FIFO and AG enable */
		LCG_REG(LCG_REG_GRAPHICS_INT_STATUS) = 0xffffffff;
		LCG_REG(LCG_REG_GRAPHICS_INT_SET_ENABLE) = 0;
		LCG_REG(LCG_REG_GRAPHICS_INT_CLR_ENABLE) = 0xffffffff;
		LCG_REG(LCG_REG_LCG_GO) = 3; /* FIFO and AG go */
//		LCG_REG(LCG_REG_BREAKPT_ADDRESS) = 0x2fffffff;

	} else {
		lcgaddr = (void *)virtual_avail;
		virtual_avail += lcg_fb_size + LCG_FONT_STORAGE_SIZE;
		ioaccess((vaddr_t)lcgaddr, LCG_FB_ADDR, (lcg_fb_size/VAX_NBPG));

		lutaddr = (void *)virtual_avail;
		virtual_avail += LCG_LUT_SIZE;
		ioaccess((vaddr_t)lutaddr, LCG_LUT_ADDR, (LCG_LUT_SIZE/VAX_NBPG));

		fifoaddr = (long*)virtual_avail;
		virtual_avail += LCG_FIFO_WIN_SIZE;
		ioaccess((vaddr_t)fifoaddr, LCG_FIFO_WIN_ADDR, (LCG_FIFO_WIN_SIZE/VAX_NBPG));
	}

	bzero(lutaddr, LCG_LUT_SIZE);
	for (i = 0; i < 8; ++i) {
		lcg_cmap.r[i] = lcg_default_cmap.r[i];
		lcg_cmap.g[i] = lcg_default_cmap.g[i];
		lcg_cmap.b[i] = lcg_default_cmap.b[i];
		lutaddr[i * 8 + 1] = i;
		lutaddr[i * 8 + 2] = 1;
		lutaddr[i * 8 + 3] = lcg_cmap.r[i] >> (lcg_depth & 7);
		lutaddr[i * 8 + 4] = 1;
		lutaddr[i * 8 + 5] = lcg_cmap.g[i] >> (lcg_depth & 7);
		lutaddr[i * 8 + 6] = 1;
		lutaddr[i * 8 + 7] = lcg_cmap.b[i] >> (lcg_depth & 7);
	}

	/*
	 * 0xf100165b 4000/60
	 * 1111 0001 0000 0000 0001 0110 0101 1011
	 * vvvv vvvv vvvv vvvv vvvv vvvv vvvv vvvv
	 * 3322 2222 2222 1111 1111 11
	 * 1098 7654 3210 9876 5432 1098 7654 3210
	 *
 	 * 0xf1001d7b 4000/VLC
	 * 1111 0001 0000 0000 0001 1101 0111 1011
	 * vvvv vvvv vvvv vvvv vvvv vvvv vvvv vvvv
	 * 3322 2222 2222 1111 1111 11
	 * 1098 7654 3210 9876 5432 1098 7654 3210
	 *
	 * 31-30     11 Vertical state
	 * 29-38     11 Horizontal state
	 * 27-26     00
	 * 25         0 console LUT select
	 * 24         1 control LUT select
	 * 23         0
	 * 22         0 cursor active
	 * 21-16 000000
	 * 15         0 video subsystem reset
	 * 14         0
	 * 13         0 LUT load size 2KB
	 * 12         1 enable H sync
	 * 11         0 Full LUT load		 1
	 * 10         1 video clock select
	 *  9- 8     10 memory refresh rate	01
	 *  7- 6     01 video refresh rate
	 *  5         0 load select		 1
	 *  4         1 read cursor output
	 *  3         1 LUT load enable
	 *  2         0 cursor enable
	 *  1         1 video enable
	 *  0         1 refresh clock enable
	 */
	/* prepare video_config reg for LUT relaod */
	video_conf
		 = (3 << 30) /* vertical state */
		 | (3 << 28) /* horizontal state */
		 | (0 << 26) /* unused */
		 | (0 << 25) /* console LUT select */
		 | (0 << 24) /* control LUT select */
		 | (0 << 23) /* unused */
		 | (0 << 22) /* cursor active */
		 | (0 << 16) /* current cursor scanline showing */
		 | (0 << 15) /* video subsystem reset */
		 | (0 << 14) /* unused */
		 | (1 << 13) /* LUT load size 2 KB */
		 | (1 << 12) /* enable horizontal sync */
		 | (1 << 10) /* video clock select */
		 | (1 << 6) /* video refresh select */
		 | (1 << 4) /* read curosr output */
		 | (1 << 3) /* LUT load enable */
		 | (0 << 2) /* cursor enable */
		 | (1 << 1) /* video enable */
		 | (1 << 0); /* refresh clock enable */
	/* FIXME needs updating for all supported models */
	if (lcg_xsize == 1280) {		/* 4000/60 HR 4PLN */
		video_conf |= (0 << 11); /* split LUT load */
		video_conf |= (2 << 8); /* memory refresh select */
		video_conf |= (0 << 5); /* split shift register load */
	} else {				/* 4000/VLC LR 8PLN */
		video_conf |= (1 << 11); /* Full LUT load */
		video_conf |= (1 << 8); /* memory refresh select */
		video_conf |= (1 << 5); /* split shift register load */
	}

	LCG_REG(LCG_REG_VIDEO_CONFIG) = video_conf;
	/* vital !!! */
	LCG_REG(LCG_REG_LUT_CONSOLE_SEL) = 1;
	LCG_REG(LCG_REG_LUT_COLOR_BASE_W) = LCG_LUT_OFFSET;

	delay(1000);
	LCG_REG(LCG_REG_LUT_CONSOLE_SEL) = 0;

#ifdef LCG_DEBUG
	if (self != NULL)
		printf("%s: video config register set 0x%08lx\n", video_conf, self->dv_xname);
#endif
}
