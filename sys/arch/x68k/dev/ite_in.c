/*	$NetBSD: ite_in.c,v 1.3 1996/10/13 03:34:56 christos Exp $	*/

/*
 * Copyright (c) 1993, 1994 Masaru Oki.
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
 *    must display the following acknowledgement:
 *	This product includes software developed by Masaru Oki.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * ITE X680x0 internal video support.
 */
#include "ite.h"
#if NITE > 0

#include <sys/param.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>

#include <machine/cpu.h>

#include <x68k/dev/itevar.h>
#include <x68k/dev/iteioctl.h>
#include <x68k/x68k/iodevice.h>

/* XXX */
#include <x68k/dev/grfioctl.h>
#include <x68k/dev/grfvar.h>

#define CRTC    (IODEVbase->io_crtc)

extern unsigned char kernel_font_width, kernel_font_height;
extern unsigned char kernel_font_lo, kernel_font_hi;
extern unsigned char kernel_font[], kernel_cursor[];

typedef struct ite_priv {
	u_char **row_ptr;			  /* array of pointers into the bitmap  */
	u_long row_bytes;
	u_long cursor_opt;
  
	/* these are precalc'ed for the putc routine so it can be faster. */
	u_int  *column_offset;			/* array of offsets for columns */
	u_int  row_offset;			/* the row offset */
	u_short width;				/* the bitmap width */
	u_short underline;			/* where the underline goes */
	u_short ft_x;				/* the font width */
	u_short ft_y;				/* the font height */
	u_char *font_cell[256];			/* the font pointer */
} ipriv_t;

static ipriv_t x68k_priv;

extern struct itesw itesw[];

#if ITEKANJI
unsigned char kern_font[16 * 256]; /* XXX for 8x16 rom font */
u_int column[1024];
u_char *row_ptr[1024];
#endif

/* keyboard LED status variable */
unsigned char kbdled;

/* 8-by-N routines */
static void ite_8n_cursor __P((struct ite_softc *ip, int flag));
static void ite_8n_putc __P((struct ite_softc *ip, int c, int dy, int dx, int mode));
/*static void ite_8n_clear __P((struct ite_softc *ip, int sy, int sx, int h, int w));*/
static void ite_8n_scroll __P((struct ite_softc *ip, int sy, int sx, int count, int dir));

/* (M<=8)-by-N routines */
static void view_le32n_cursor __P((struct ite_softc *ip, int flag));
static void view_le8n_putc __P((struct ite_softc *ip, int c, int dy, int dx, int mode));
static void view_le8n_clear __P((struct ite_softc *ip, int sy, int sx, int h, int w));
static void view_le8n_scroll __P((struct ite_softc *ip, int sy, int sx, int count, int dir));
#if 0
void scroll_bitmap __P((bmap_t *bm, u_short x, u_short y, u_short width, u_short height, short dx, short dy, u_char mask));
#endif


/* globals */

int ite_default_x;
int ite_default_y;
int ite_default_width = 768;
int ite_default_height = 512;
int ite_default_depth = 4; /*2;*/

/* raster copy routines */
/*static void vsyncchk(void);*/
static void txrascpy(unsigned short src_dst, short copy, signed short mode);

#define IPUNIT(ip) (((u_long)ip-(u_long)ite_softc)/sizeof(struct ite_softc))

#if x68k /* old defines ... XXX */
#define FB_WIDTH 1024
#endif

int 
ite_newsize (ip, winsz)
	struct ite_softc *ip;
	struct itewinsize *winsz;
{
	struct grf_softc *gp = ip->grf;
	ipriv_t *cci = ip->priv;
	u_long fbp, i;
	int error;

	ip->cols = (gp->g_display.gd_dwidth)  / ip->ftwidth;
	ip->rows = (gp->g_display.gd_dheight) / ip->ftheight;

	/*
	 * save new values so that future opens use them
	 * this may not be correct when we implement Virtual Consoles
	 */
	ite_default_height = gp->g_display.gd_dheight;
	ite_default_width = gp->g_display.gd_dwidth;
	ite_default_x = 0;
	ite_default_y = 0;
	ite_default_depth = gp->g_display.gd_planes;

	cci->row_ptr = row_ptr;
	cci->column_offset = column;
    
	if (!cci->row_ptr || cci->column_offset == NULL)
		panic ("no memory for console device.");

	cci->width = gp->g_display.gd_dwidth;
	cci->underline = ip->ftbaseline + 1;
	cci->row_offset = gp->g_display.gd_fbwidth / 8;
	cci->ft_x = ip->ftwidth;
	cci->ft_y = ip->ftheight;
 
	cci->row_bytes = cci->row_offset * ip->ftheight;

	cci->row_ptr[0] = gp->g_fbkva;
	for (i = 1; i < ip->rows; i++) 
		cci->row_ptr[i] = cci->row_ptr[i-1] + cci->row_bytes;

	/* initialize the column offsets */
	cci->column_offset[0] = 0;
	for (i = 1; i < ip->cols; i++) 
		cci->column_offset[i] = cci->column_offset[i-1] + cci->ft_x;

	/* initialize the font cell pointers */
	cci->font_cell[ip->font_lo] = ip->font;
	for (i=ip->font_lo+1; i<=ip->font_hi; i++)
		cci->font_cell[i] = cci->font_cell[i-1] + ip->ftheight;
	    
	return (error);
}

void
view_init(ip)
	register struct ite_softc *ip;
{
	struct itewinsize wsz;
	struct itesw *sp = itesw;
	ipriv_t *cci = ip->priv;

	if (cci)
		return;

#if ITEKANJI
	ip->font     = kern_font;
	ip->font_lo  = 0;
	ip->font_hi  = 255;
	ip->ftwidth  = 8;
	ip->ftheight = 16;
	ip->ftbaseline = 14;
	ip->cursor   = kernel_cursor;
	bcopy((void *)IODEVbase->cgrom0_8x16, kern_font, sizeof (kern_font));
#else
	ip->font     = kernel_font;
	ip->font_lo  = kernel_font_lo;
	ip->font_hi  = kernel_font_hi;
	ip->ftwidth  = kernel_font_width;
	ip->ftheight = kernel_font_height;
	ip->ftbaseline = kernel_font_baseline;
	ip->ftboldsmear = kernel_font_boldsmear;
#endif

#if x68k /* XXX */
		sp->ite_cursor = (void*)ite_8n_cursor;
		sp->ite_putc = (void*)ite_8n_putc;
		sp->ite_clear = (void*)view_le8n_clear;
		sp->ite_scroll = (void*)ite_8n_scroll;
#else
	/* Find the correct set of rendering routines for this font.  */
	if (ip->ftwidth <= 8) {
		sp->ite_cursor = (void*)view_le32n_cursor;
		sp->ite_putc = (void*)view_le8n_putc;
		sp->ite_clear = (void*)view_le8n_clear;
		sp->ite_scroll = (void*)view_le8n_scroll;
	} else { 
		panic("kernel font size not supported");
	}
#endif

	cci = &x68k_priv;

	ip->priv = cci;
	cci->cursor_opt = 0;
	cci->row_ptr = NULL;
	cci->column_offset = NULL;

	wsz.x = ite_default_x;
	wsz.y = ite_default_y;
	wsz.width = ite_default_width;
	wsz.height = ite_default_height;
	wsz.depth = ite_default_depth;

	kbdled = 0xff;
	ite_newsize (ip, &wsz);

#define RED   (0x1f << 6)
#define BLUE  (0x1f << 1)
#define GREEN (0x1f << 11)
	/*
	 * XXX intialize colormap
	 */
	IODEVbase->tpalet[0] = 0;			/* black */
	IODEVbase->tpalet[1] = 1 | RED;			/* red */
	IODEVbase->tpalet[2] = 1 | GREEN;		/* green */
	IODEVbase->tpalet[3] = 1 | RED | GREEN;		/* yellow */
	IODEVbase->tpalet[4] = 1 | BLUE;		/* blue */
	IODEVbase->tpalet[5] = 1 | BLUE | RED;		/* magenta */
	IODEVbase->tpalet[6] = 1 | BLUE | GREEN;	/* cyan */
	IODEVbase->tpalet[7] = 1 | BLUE | RED | GREEN;	/* white */

#if 0
	cc_mode(ip->grf, GM_GRFON, NULL, 0, 0);
#endif
}

void
view_deinit(ip)
	struct ite_softc *ip;
{
	ip->flags &= ~ITE_INITED;
	mfp.udr = 0x48;     /* send character from keyboard disable */
}

#if 0
static __inline void
vsyncchk (void)
{
  while(!(mfp.gpip & 0x10)) asm("nop");
  while((mfp.gpip & 0x10)) asm("nop");
}
#endif

/*** 8-by-N routines ***/

static inline void
ite_8n_windowmove (src, srcx, srcy, srcmod,
		    dst, dstx, dsty, dstmod, h, w, op)
	unsigned char *src, *dst;
	unsigned short srcx, srcy, srcmod;
	unsigned short dstx, dsty, dstmod;
	unsigned short h, w;
	unsigned char op;
{
	short i;	/* NOT unsigned! */
	unsigned char h1;

	src += srcmod * srcy + (srcx >> 3);
	dst += dstmod * dsty + (dstx >> 3);

#if 0
	printf("ccwm: %x-%x-%x-%x-%c\n", src, dst, h, w,
	       op == RR_XOR ? '^' : op == RR_COPY ? '|' : op == RR_CLEAR ? 'C' : 'I');
#endif

#ifdef DIAGNOSTIC  
	/* currently, only drawing to byte slots is supported... */
	if ((srcx & 07) || (dstx & 07) || (w & 07))
		panic ("ite_windowmove: odd offset");
#endif

	w >>= 3;

	/*
	 * Ok, this is nastier than it could be to help the optimizer unroll
	 * loops for the most common case of 8x8 characters.
	 *
	 * Note that bzero() does some clever optimizations for large range
	 * clears, so it should pay the subroutine call.
	 */

/* perform OP on one bit row of data. */
#define ONEOP(dst, src, op) \
	do { if ((src) > (dst))					\
	  for (i = 0; i < w; i++) (dst)[i] op (src)[i];		\
	else							\
	  for (i = w - 1; i >= 0; i--) (dst)[i] op (src)[i]; } while (0)

/*
 * perform a block of eight ONEOPs. This enables the optimizer to unroll
 * the for-statements, as they have a loop counter known at compiletime
 */
#define EIGHTOP(dst, src, op) \
	for (h1 = 0; h1 < 8; h1++, src += srcmod, dst += dstmod) \
	    ONEOP (dst, src, op);
	  
	switch (op) {
	case RR_COPY:
		for (; h >= 8; h -= 8)
			EIGHTOP (dst, src, =);
		for (; h > 0; h--, src += srcmod, dst += dstmod)
			ONEOP (dst, src, =);
		break;
	  
	case RR_CLEAR:
		for (; h >= 8; h -= 8)
			for (h1 = 0; h1 < 8; h1++, dst += dstmod)
				bzero (dst, w);
		for (; h > 0; h--, dst += dstmod)
			bzero (dst, w);
		break;
	  
	case RR_XOR:
		for (; h >= 8; h -= 8)
			EIGHTOP (dst, src, ^=);
		for (; h > 0; h--, src += srcmod, dst += dstmod)
			ONEOP (dst, src, ^=);
		break;
	  
	case RR_COPYINVERTED:
		for (; h >= 8; h -= 8)
			EIGHTOP (dst, src, =~);
		for (; h > 0; h--, src += srcmod, dst += dstmod)
			ONEOP (dst, src, =~);
		break;
	}
}


static void
ite_8n_cursor(ip, flag)
	register struct ite_softc *ip;
        register int flag;
{
	/* the cursor is always drawn in the last plane */
	unsigned char *ovplane, opclr, opset;
  
	ovplane = (void *)IODEVbase->tvram;

#if 1
	if (flag == START_CURSOROPT)
		flag = ERASE_CURSOR;
	else if (flag == END_CURSOROPT)
		flag = DRAW_CURSOR;
	else
		flag = START_CURSOROPT;
#endif

	if (flag == START_CURSOROPT || flag == END_CURSOROPT)
		return;

	/* if drawing into an overlay plane, don't xor, clr and set */
	if (0) {
		opclr = RR_CLEAR; opset = RR_COPY;
	} else {
		opclr = opset = RR_XOR;
	}

	if (flag != DRAW_CURSOR) {
		/* erase it */
		ite_8n_windowmove (ip->cursor, 0, 0, 1,
				   ovplane, ip->cursorx * ip->ftwidth,
				   ip->cursory * ip->ftheight,
				   FB_WIDTH >> 3,
				   ip->ftheight, ip->ftwidth, opclr);
	}
	if (flag == DRAW_CURSOR || flag == MOVE_CURSOR) {
		/* draw it */
		int newx = min(ip->curx, ip->cols - 1);
		ite_8n_windowmove (ip->cursor, 0, 0, 1,
				   ovplane, newx * ip->ftwidth,
				   ip->cury * ip->ftheight,
				   FB_WIDTH >> 3,
				   ip->ftheight, ip->ftwidth, opset);
		ip->cursorx = newx;
		ip->cursory = ip->cury;
	}
}

typedef void in_putc_func __P((ipriv_t *, u_char *, u_char *, u_int, u_int, u_int, u_int));
in_putc_func *put_func[ATTR_ALL+1];

typedef void in_wputc_func __P((ipriv_t *, u_char *, u_short *, u_int, u_int, u_int, u_int));
in_wputc_func *wput_func[ATTR_ALL+1];

static void
ite_8n_putc(ip, c, dy, dx, mode)
	struct ite_softc *ip;
	int dy, dx;
	int c, mode;
{
	ipriv_t *cci = ip->priv;
	int plane;
	u_char *row = cci->row_ptr[dy];
	u_int column = cci->column_offset[dx];

	if (ip->save_char) {
		/*
		 * multibyte character putc
		 */
		char *kanji_font;
		unsigned short lc = ip->save_char & 0x7f;
		unsigned short rc = c & 0x7f;

		/* undefined code ? */
		if (lc < 0x21 || 0x7e < lc || rc < 0x21 || 0x7e < rc) {
			lc = 0x22;
			rc = 0x28;
		}

		rc -= 0x21;

		if (lc < 0x30)		/* not kanji */
			kanji_font = (void *)&IODEVbase->cgrom0_16x16[(lc-0x21)*32*0x5e];
		else if (lc < 0x50)	/* kanji class 1 */
			kanji_font = (void *)&IODEVbase->cgrom1_16x16[(lc-0x30)*32*0x5e];
		else			/* kanji class 2 */
			kanji_font = (void *)&IODEVbase->cgrom2_16x16[(lc-0x50)*32*0x5e];

#if ITEKANJI /* XXX */
		kanji_font += (rc * 16 * 2);
#else
		kanji_font += (rc * ip->ftheight * 2);
#endif
#if 0
		for (plane = 0; plane < 4; plane++) /* XXX */
			wput_func[mode](cci, row + 0x20000*plane,
					(ip->fgcolor & (1 << plane)) ? kanji_font
					: &IODEVbase->cgrom0_16x16[0],
					column,
					cci->row_offset,
					cci->ft_x * 2,
					cci->ft_y);
#else
		CRTC.r21 = 0x0100 | ip->fgcolor << 4;
		wput_func[mode](cci, row, (u_short *)kanji_font,
				column,
				cci->row_offset,
				16,
				16);
		CRTC.r21 ^= 0x00F0;
		wput_func[mode](cci, row,
				(u_short *)&IODEVbase->cgrom0_16x16[0],
				column,
				cci->row_offset,
				16,
				16);
		CRTC.r21 = 0;
#endif
		return;
	}

	/* single byte character (ASCII) */
	if (*ip->GL == CSET_JISKANA)
		c |= 0x80;
	if (c >= ip->font_lo && c <= ip->font_hi) {
#if 0
		for (plane = 0; plane < 4; plane++) /* XXX */
			put_func[mode](cci,
				       row + 0x20000*plane,
				       cci->font_cell[(ip->fgcolor & (1 << plane)) ? c : 0x20],
				       column,
				       cci->row_offset,
				       cci->ft_x,
				       cci->ft_y);
#else
		CRTC.r21 = 0x0100 | ip->fgcolor << 4;
		put_func[mode](cci, row,
			       cci->font_cell[c],
			       column,
			       cci->row_offset,
			       cci->ft_x,
			       cci->ft_y);
		CRTC.r21 ^= 0x00f0;
		put_func[mode](cci, row,
#if 1
				(void *)&IODEVbase->cgrom0_16x16[0],
#else
			       cci->font_cell[0x20],
#endif
			       column,
			       cci->row_offset,
			       8/*cci->ft_x*/,
			       16/*cci->ft_y*/);
		CRTC.r21 = 0;
#endif
	}
}

/* Note: sx is only relevant for SCROLL_LEFT or SCROLL_RIGHT.  */
static void
ite_8n_scroll(ip, sy, sx, count, dir)
        register struct ite_softc *ip;
        register int sy;
        int dir, sx, count;
{
	ipriv_t *cci = ip->priv;

	if (dir == SCROLL_UP) {
		int dy = sy - count;
		int bot = ip->bottom_margin;
		int height = bot - sy + 1;

#if 1
		txrascpy(((sy * 4) << 8) | dy * 4, height * 4 , 0x0f);
#else
		ite_8n_windowmove(cci->row_ptr[0],
				  0, sy * ip->ftheight, FB_WIDTH >> 3,
				  cci->row_ptr[0],
				  0, dy * ip->ftheight, FB_WIDTH >> 3,
				  height * ip->ftheight, ip->cols * ip->ftwidth, RR_COPY);
#endif
	} else if (dir == SCROLL_DOWN) {
		int dy = sy + count;
		int bot = ip->bottom_margin;
		int height = bot - dy + 1;
		int i;

#if 1
		txrascpy((((sy+height-1)*4+3)<<8) | (dy+height-1)*4+3,
			 height * 4 , 0x0f | 0x8000);
#else
		for (i = (height - 1); i >= 0; i--)
			ite_8n_windowmove(cci->row_ptr[0],
					  0, (sy + i) * ip->ftheight, FB_WIDTH >> 3,
					  cci->row_ptr[0],
					  0, (dy + i) * ip->ftheight, FB_WIDTH >> 3,
					  ip->ftheight, ip->cols * ip->ftwidth, RR_COPY);
#endif
	}
	else if (dir == SCROLL_RIGHT) {
		ite_8n_windowmove(cci->row_ptr[sy],
				  sx * ip->ftwidth, 0, FB_WIDTH >> 3,
				  cci->row_ptr[sy],
				  (sx + count) * ip->ftwidth, 0, FB_WIDTH >> 3,
				  ip->ftheight, (ip->cols - (sx + count)) * ip->ftwidth,
				  RR_COPY);
		ite_8n_windowmove(cci->row_ptr[sy] + 0x20000,
				  sx * ip->ftwidth, 0, FB_WIDTH >> 3,
				  cci->row_ptr[sy] + 0x20000,
				  (sx + count) * ip->ftwidth, 0, FB_WIDTH >> 3,
				  ip->ftheight, (ip->cols - (sx + count)) * ip->ftwidth,
				  RR_COPY);
		ite_8n_windowmove(cci->row_ptr[sy] + 0x40000,
				  sx * ip->ftwidth, 0, FB_WIDTH >> 3,
				  cci->row_ptr[sy] + 0x40000,
				  (sx + count) * ip->ftwidth, 0, FB_WIDTH >> 3,
				  ip->ftheight, (ip->cols - (sx + count)) * ip->ftwidth,
				  RR_COPY);
		ite_8n_windowmove(cci->row_ptr[sy] + 0x60000,
				  sx * ip->ftwidth, 0, FB_WIDTH >> 3,
				  cci->row_ptr[sy] + 0x60000,
				  (sx + count) * ip->ftwidth, 0, FB_WIDTH >> 3,
				  ip->ftheight, (ip->cols - (sx + count)) * ip->ftwidth,
				  RR_COPY);
	} else {
		ite_8n_windowmove(cci->row_ptr[sy],
				  sx * ip->ftwidth, 0, FB_WIDTH >> 3,
				  cci->row_ptr[sy],
				  (sx - count) * ip->ftwidth, 0, FB_WIDTH >> 3,
				  ip->ftheight, (ip->cols - sx) * ip->ftwidth, RR_COPY);
		ite_8n_windowmove(cci->row_ptr[sy] + 0x20000,
				  sx * ip->ftwidth, 0, FB_WIDTH >> 3,
				  cci->row_ptr[sy] + 0x20000,
				  (sx - count) * ip->ftwidth, 0, FB_WIDTH >> 3,
				  ip->ftheight, (ip->cols - sx) * ip->ftwidth, RR_COPY);
		ite_8n_windowmove(cci->row_ptr[sy] + 0x40000,
				  sx * ip->ftwidth, 0, FB_WIDTH >> 3,
				  cci->row_ptr[sy] + 0x40000,
				  (sx - count) * ip->ftwidth, 0, FB_WIDTH >> 3,
				  ip->ftheight, (ip->cols - sx) * ip->ftwidth, RR_COPY);
		ite_8n_windowmove(cci->row_ptr[sy] + 0x60000,
				  sx * ip->ftwidth, 0, FB_WIDTH >> 3,
				  cci->row_ptr[sy] + 0x60000,
				  (sx - count) * ip->ftwidth, 0, FB_WIDTH >> 3,
				  ip->ftheight, (ip->cols - sx) * ip->ftwidth, RR_COPY);
	}
}


/*** (M<8)-by-N routines ***/

#if 0
/*
 * NOTE: This routine assumes a cursor overlay plane,
 * but it does allow cursors up to 32 pixels wide.
 */
static void
view_le32n_cursor(struct ite_softc *ip, int flag)
{
	ipriv_t  *cci = (ipriv_t *) ip->priv;
	view_t *v = cci->view;
	int dr_plane = 3; /* XXX 4planes, 4-1 */
	int cend, ofs, h, cstart;
	unsigned char opclr, opset;
	u_char *pl;

	if (flag == START_CURSOROPT) {
		if (!cci->cursor_opt) {
			view_le32n_cursor (ip, ERASE_CURSOR);
		}
		cci->cursor_opt++;
		return;				  /* if we are already opted. */
	} else if (flag == END_CURSOROPT) {
		cci->cursor_opt--;
		
	}
    
	if (cci->cursor_opt) 
		return;				  /* if we are still nested. */
						  /* else we draw the cursor. */

	cstart = 0;
	cend = ip->ftheight-1; 

	/* erase cursor */
	if (flag != DRAW_CURSOR && flag != END_CURSOROPT) {
		/* erase the cursor */
		int h;
		if (dr_plane) {
			for (h = cend; h >= 0; h--) {
				asm("bfclr %0@{%1:%2}"
				    : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
				pl += cci->row_offset;
	    }
		} else {
			for (h = cend; h >= 0; h--) {
				asm("bfchg %0@{%1:%2}"
				    : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
				pl += cci->row_offset;
			}
		}
	}

	if ((flag == DRAW_CURSOR || flag == MOVE_CURSOR || flag == END_CURSOROPT)) {

		ip->cursorx = min(ip->curx, ip->cols-1);
		ip->cursory = ip->cury;
		cstart = 0;
		cend = ip->ftheight-1; 
		pl = VDISPLAY_LINE (v, dr_plane, (ip->cursory*ip->ftheight+cstart));
		ofs = (ip->cursorx * ip->ftwidth);

		/* draw the cursor */
		if (dr_plane) {
			for (h = cend; h >= 0; h--) {
				asm("bfset %0@{%1:%2}"
				    : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
				pl += cci->row_offset;
			}
		} else {
			for (h = cend; h >= 0; h--) {
				asm("bfchg %0@{%1:%2}"
				    : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
				pl += cci->row_offset;
			}
		}
	}
}
#endif


static inline int
expbits (data)
	int data;
{
	int i, nd = 0;
	if (data & 1)
		nd |= 0x02;
	for (i=1; i < 32; i++) {
		if (data & (1 << i))
			nd |= 0x5 << (i-1);
	}
	nd &= ~data;
	return (~nd);
}


/* Notes: optimizations given the kernel_font_(width|height) #define'd.
 *        the dbra loops could be elminated and unrolled using height,
 *        the :width in the bfxxx instruction could be made immediate instead
 *        of a data register as it now is.
 *        the underline could be added when the loop is unrolled
 *
 *        It would look like hell but be very fast.*/
 
static void 
putc_nm(cci, p, f, co, ro, fw, fh)
	register ipriv_t *cci;
	register u_char  *p;
	register u_char  *f;
	register u_int    co;
	register u_int    ro;
	register u_int    fw;
	register u_int    fh;
{
#if ITEKANJI /* XXX */
	p += co / 8;
	for (fh = 0; fh < 16; fh++) {
		asm ("movb %0,%1@" : /* no outout */ :
		     "d" (*f++), "a" (p));
		p += ro;
	}
#else
	while (fh--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (*f++), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
#endif
}

static void 
putc_in(cci, p, f, co, ro, fw, fh)
	register ipriv_t *cci;
	register u_char  *p;
	register u_char  *f;
	register u_int    co;
	register u_int    ro;
	register u_int    fw;
	register u_int    fh;
{
#if ITEKANJI /* XXX */
	fh = 16;
	p += co / 8;
	for (fh = 0; fh < 16; fh++) {
		asm ("movb %0,%1@" : /* no outout */ :
		     "d" (~(*f++)), "a" (p));
		p += ro;
	}
#else
	while (fh--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(*f++)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
#endif
}


static void 
putc_ul(cci, p, f, co, ro, fw, fh)
	register ipriv_t *cci;
	register u_char  *p;
	register u_char  *f;
	register u_int    co;
	register u_int    ro;
	register u_int    fw;
	register u_int    fh;
{
	int underline = cci->underline;
	while (underline--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (*f++), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}

	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (expbits(*f++)), "a" (p), "d" (co), "d" (fw));
	p += ro;

	underline = fh - cci->underline - 1;
	while (underline--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (*f++), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
}


static void 
putc_ul_in(cci, p, f, co, ro, fw, fh)
	register ipriv_t *cci;
	register u_char  *p;
	register u_char  *f;
	register u_int    co;
	register u_int    ro;
	register u_int    fw;
	register u_int    fh;
{
	int underline = cci->underline;
	while (underline--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(*f++)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}

	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~expbits(*f++)), "a" (p), "d" (co), "d" (fw));
	p += ro;

	underline = fh - cci->underline - 1;
	while (underline--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(*f++)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
}

/* bold */
static void 
putc_bd(cci, p, f, co, ro, fw, fh)
	ipriv_t *cci;
	u_char  *p;
	u_char  *f;
	u_int    co;
	u_int    ro;
	u_int    fw;
	u_int    fh;
{
	u_short ch;
#ifdef ITEKANJI /* XXX */
	p += co / 8;
	for (fh = 0; fh < 16; fh++) {
		ch = *f++;
		ch |= ch >> 1;
		asm ("movb %0,%1@" : /* no outout */ :
		     "d" (ch), "a" (p));
		p += ro;
	}
#else

	fw++;
	while (fh--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (ch), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
#endif
}

static void 
putc_bd_in(cci, p, f, co, ro, fw, fh)
	ipriv_t *cci;
	u_char  *p;
	u_char  *f;
	u_int    co;
	u_int    ro;
	u_int    fw;
	u_int    fh;
{
	u_short ch;

#ifdef ITEKANJI /* XXX */
	p += co / 8;
	for (fh = 0; fh < 16; fh++) {
		ch = *f++;
		ch |= ch >> 1;
		asm ("movb %0,%1@" : /* no outout */ :
		     "d" (~ch), "a" (p));
		p += ro;
	}
#else
	fw++;
	while (fh--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(ch)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
#endif
}


static void 
putc_bd_ul(cci, p, f, co, ro, fw, fh)
	ipriv_t *cci;
	u_char  *p;
	u_char  *f;
	u_int    co;
	u_int    ro;
	u_int    fw;
	u_int    fh;
{
	int underline = cci->underline;
	u_short ch;
#ifdef ITEKANJI /* XXX */
	fh = 16;
#endif

	fw++;
	while (underline--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (ch), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}

	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (expbits(ch)), "a" (p), "d" (co), "d" (fw));
	p += ro;

	underline = fh - cci->underline - 1;
	while (underline--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (ch), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
}


static void 
putc_bd_ul_in(cci, p, f, co, ro, fw, fh)
	ipriv_t *cci;
	u_char  *p;
	u_char  *f;
	u_int    co;
	u_int    ro;
	u_int    fw;
	u_int    fh;
{
	int underline = cci->underline;
	u_short ch;
#ifdef ITEKANJI /* XXX */
	fh = 16;
#endif
    
	fw++;
	while (underline--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(ch)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}

	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~expbits(ch)), "a" (p), "d" (co), "d" (fw));
	p += ro;

	underline = fh - cci->underline - 1;
	while (underline--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(ch)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
}

in_putc_func *put_func[ATTR_ALL+1] = {
	putc_nm,
	putc_in,
	putc_ul,
	putc_ul_in,
	putc_bd,
	putc_bd_in,
	putc_bd_ul,
	putc_bd_ul_in,
/* no support for blink */
	putc_nm,
	putc_in,
	putc_ul,
	putc_ul_in,
	putc_bd,
	putc_bd_in,
	putc_bd_ul,
	putc_bd_ul_in
};

static void 
wputc_nm(cci, p, f, co, ro, fw, fh)
	register ipriv_t *cci;
	register u_char  *p;
	register u_short *f;
	register u_int    co;
	register u_int    ro;
	register u_int    fw;
	register u_int    fh;
{
#if ITEKANJI /* XXX */
	p += co / 8;
	for (fh = 0; fh < 16; fh++) {
		asm ("movw %0,%1@" : /* no outout */ :
		     "d" (*f++), "a" (p));
		p += ro;
	}
#else
	while (fh--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (*f++), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
#endif
}

static void 
wputc_in(cci, p, f, co, ro, fw, fh)
	register ipriv_t *cci;
	register u_char  *p;
	register u_short *f;
	register u_int    co;
	register u_int    ro;
	register u_int    fw;
	register u_int    fh;
{
#if ITEKANJI /* XXX */
	p += co / 8;
	for (fh = 0; fh < 16; fh++) {
		asm ("movw %0,%1@" : /* no outout */ :
		     "d" (~(*f++)), "a" (p));
		p += ro;
	}
#else
	while (fh--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(*f++)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
#endif
}


static void 
wputc_ul (cci,p,f,co,ro,fw,fh)
	register ipriv_t *cci;
	register u_char  *p;
	register u_short *f;
	register u_int    co;
	register u_int    ro;
	register u_int    fw;
	register u_int    fh;
{
	int underline = cci->underline;
	while (underline--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (*f++), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
	
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (expbits(*f++)), "a" (p), "d" (co), "d" (fw));
	p += ro;
	
	underline = fh - cci->underline - 1;
	while (underline--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (*f++), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
}


static void 
wputc_ul_in (cci,p,f,co,ro,fw,fh)
	register ipriv_t *cci;
	register u_char  *p;
	register u_short *f;
	register u_int    co;
	register u_int    ro;
	register u_int    fw;
	register u_int    fh;
{
	int underline = cci->underline;
	while (underline--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(*f++)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}

	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~expbits(*f++)), "a" (p), "d" (co), "d" (fw));
	p += ro;

	underline = fh - cci->underline - 1;
	while (underline--) {
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(*f++)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
}

/* bold */
static void 
wputc_bd (cci, p, f, co, ro, fw, fh)
	ipriv_t *cci;
	u_char  *p;
	u_short *f;
	u_int    co;
	u_int    ro;
	u_int    fw;
	u_int    fh;
{
	u_short ch;

#if ITEKANJI /* XXX */
	p += co / 8;
	for (fh = 0; fh < 16; fh++) {
		ch = *f++;
		ch |= ch >> 1;
		asm ("movw %0,%1@" : /* no outout */ :
		     "d" (ch), "a" (p));
		p += ro;
	}
#else
	fw++;
	while (fh--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (ch), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
#endif
}

static void 
wputc_bd_in (cci, p, f, co, ro, fw, fh)
	ipriv_t *cci;
	u_char  *p;
	u_short *f;
	u_int    co;
	u_int    ro;
	u_int    fw;
	u_int    fh;
{
	u_short ch;

#if ITEKANJI /* XXX */
	p += co / 8;
	for (fh = 0; fh < 16; fh++) {
		ch = *f++;
		ch |= ch >> 1;
		asm ("movw %0,%1@" : /* no outout */ :
		     "d" (~ch), "a" (p));
		p += ro;
	}
#else    
	fw++;
	while (fh--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~ch), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
#endif
}


static void 
wputc_bd_ul (cci,p,f,co,ro,fw,fh)
	ipriv_t *cci;
	u_char  *p;
	u_short *f;
	u_int    co;
	u_int    ro;
	u_int    fw;
	u_int    fh;
{
	int underline = cci->underline;
	u_short ch;

	fw++;
	while (underline--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (ch), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}

	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (expbits(ch)), "a" (p), "d" (co), "d" (fw));
	p += ro;

	underline = fh - cci->underline - 1;
	while (underline--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (ch), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
}


static void 
wputc_bd_ul_in (cci, p, f, co, ro, fw, fh)
	ipriv_t *cci;
	u_char  *p;
	u_short *f;
	u_int    co;
	u_int    ro;
	u_int    fw;
	u_int    fh;
{
	int underline = cci->underline;
	u_short ch;
    
	fw++;
	while (underline--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(ch)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}

	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~expbits(ch)), "a" (p), "d" (co), "d" (fw));
	p += ro;

	underline = fh - cci->underline - 1;
	while (underline--) {
		ch = *f++;
		ch |= ch << 1;
		asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
		     "d" (~(ch)), "a" (p), "d" (co), "d" (fw));
		p += ro;
	}
}

in_wputc_func *wput_func[ATTR_ALL+1] = {
	wputc_nm,
	wputc_in,
	wputc_ul,
	wputc_ul_in,
	wputc_bd,
	wputc_bd_in,
	wputc_bd_ul,
	wputc_bd_ul_in,
/* no support for blink */
	wputc_nm,
	wputc_in,
	wputc_ul,
	wputc_ul_in,
	wputc_bd,
	wputc_bd_in,
	wputc_bd_ul,
	wputc_bd_ul_in
};

#if 0
/* FIX: shouldn't this advance the cursor even if the character to
        be output is not available in the font? -ch */

static void
view_le8n_putc(ip, c, dy, dx, mode)
	struct ite_softc *ip;
	int c, dy, dx, mode;
{
	register ipriv_t *cci = (ipriv_t *) ip->priv;
	if (c < ip->font_lo || c > ip->font_hi)
		return;

	put_func[mode](cci,
		       cci->row_ptr[dy],
		       cci->font_cell[c],
		       cci->column_offset[dx],
		       cci->row_offset,
		       cci->ft_x,
		       cci->ft_y);
}
#endif

static void
view_le8n_clear(ip, sy, sx, h, w)
     struct ite_softc *ip;
     int sy, sx, h, w;
{
	ipriv_t *cci = (ipriv_t *) ip->priv;
	int bytes_per_row = ip->grf->g_display.gd_dwidth / 8;
	unsigned short crtcr21 = CRTC.r21;

	CRTC.r21 = 0x01f0; /* multi page access mode */

	if ((sx == 0) && (w == ip->cols)) {
		/* common case: clearing whole lines */
		while (h--) {
			int i;
			u_char *ptr = cci->row_ptr[sy]; 
			for (i=0; i < ip->ftheight; i++) {
				bzero(ptr, bytes_per_row);
				ptr += ip->grf->g_display.gd_fbwidth / 8;
			}
			sy++;
		}
	} else {
		/* clearing only part of a line */
		/* XXX could be optimized MUCH better, but is it worth the trouble? */
		while (h--) {
			u_char *pl = cci->row_ptr[sy];
#if ITEKANJI /* XXX */
			int ofs = sx;
#else
			int ofs = sx * ip->ftwidth;
#endif
			int i, j;
			for (i = w-1; i >= 0; i--) {
				u_char *ppl = pl;
				for (j = ip->ftheight-1; j >= 0; j--) {
#if ITEKANJI /* XXX */
					asm("clrb %0@" : : "a" (ppl + ofs));
#else
					asm("bfclr %0@{%1:%2}"
					    : : "a" (ppl), "d" (ofs),
					    "d" (ip->ftwidth));
#endif
					ppl += ip->grf->g_display.gd_fbwidth / 8;
				}
#if ITEKANJI /* XXX */
				ofs++;
#else
				ofs += ip->ftwidth;
#endif
			}
			sy++;
		}
	}
	CRTC.r21 = crtcr21;
}

#if 0
/* Note: sx is only relevant for SCROLL_LEFT or SCROLL_RIGHT.  */
static void
view_le8n_scroll(ip, sy, sx, count, dir)
        register struct ite_softc *ip;
        register int sy;
        int dir, sx, count;
{
	bmap_t *bm = ((ipriv_t *)ip->priv)->view->bitmap;
	u_char *pl = ((ipriv_t *)ip->priv)->row_ptr[sy];

	if (dir == SCROLL_UP) {
		int dy = sy - count;
		int height = ip->bottom_margin - sy + 1;
		int i;
		/*FIX: add scroll bitmap call */
		view_le32n_cursor(ip, ERASE_CURSOR);
		scroll_bitmap (bm, 0, dy*ip->ftheight,
			       bm->bytes_per_row >> 3, (ip->bottom_margin-dy+1)*ip->ftheight,
			       0, -(count*ip->ftheight), 0x1);
/*	if (ip->cursory <= bot || ip->cursory >= dy) {
	ip->cursory -= count;
	} */
	} else if (dir == SCROLL_DOWN) {
		int dy = sy + count;
		int height = ip->bottom_margin - dy + 1;
		int i;

		/* FIX: add scroll bitmap call */
		view_le32n_cursor(ip, ERASE_CURSOR);
		scroll_bitmap (bm, 0, sy*ip->ftheight,
			       bm->bytes_per_row >> 3, (ip->bottom_margin-sy+1)*ip->ftheight,
			       0, count*ip->ftheight, 0x1);
/*	if (ip->cursory <= bot || ip->cursory >= sy) {
	ip->cursory += count;
	} */
	} else if (dir == SCROLL_RIGHT) {
		int sofs = (ip->cols - count) * ip->ftwidth;
		int dofs = (ip->cols) * ip->ftwidth;
		int i, j;

		view_le32n_cursor(ip, ERASE_CURSOR);
		for (j = ip->ftheight-1; j >= 0; j--) {
			int sofs2 = sofs, dofs2 = dofs;
			for (i = (ip->cols - (sx + count))-1; i >= 0; i--) {
				int t;
				sofs2 -= ip->ftwidth;
				dofs2 -= ip->ftwidth;
				asm("bfextu %1@{%2:%3},%0"
				    : "=d" (t)
				    : "a" (pl), "d" (sofs2), "d" (ip->ftwidth));
				asm("bfins %3,%0@{%1:%2}"
				    : : "a" (pl), "d" (dofs2), "d" (ip->ftwidth), "d" (t));
			}
			pl += ip->grf->g_display.gd_fbwidth / 8;
		}
	} else /* SCROLL_LEFT */
	{
		int sofs = (sx) * ip->ftwidth;
		int dofs = (sx - count) * ip->ftwidth;
		int i, j;

		view_le32n_cursor(ip, ERASE_CURSOR);
		for (j = ip->ftheight-1; j >= 0; j--) {
			int sofs2 = sofs, dofs2 = dofs;
			for (i = (ip->cols - sx)-1; i >= 0; i--) {
				int t;
				asm("bfextu %1@{%2:%3},%0"
				    : "=d" (t)
				    : "a" (pl), "d" (sofs2), "d" (ip->ftwidth));
				asm("bfins %3,%0@{%1:%2}"
				    : : "a" (pl), "d" (dofs2), "d" (ip->ftwidth), "d" (t));
				sofs2 += ip->ftwidth;
				dofs2 += ip->ftwidth;
			}
			pl += ip->grf->g_display.gd_fbwidth / 8;
		}
	}
}

void 
scroll_bitmap (bm, x, y, width, height, dx, dy, mask)
	bmap_t *bm;
	u_short x, y, width, height;
	short dx, dy;
	u_char mask;
{
	u_short depth = bm->depth; 
	u_short lwpr = bm->bytes_per_row >> 2;
	if (dx) {
		/* FIX: */
		panic ("delta x not supported in scroll bitmap yet.");
	} 
	if (bm->flags & BMF_INTERLEAVED) {
		height *= depth;
		depth = 1;
	}
	if (dy == 0) {
		return;
	}
	if (dy > 0) {
		int i;
		for (i=0; i < depth && mask; i++, mask >>= 1) {
			if (0x1 & mask) {
				u_long *pl = (u_long *)bm->plane[i];
				u_long *src_y = pl + (lwpr*y);
				u_long *dest_y = pl + (lwpr*(y+dy));
				u_long count = lwpr*(height-dy);
				u_long *clr_y = src_y;
				u_long clr_count = dest_y - src_y;
				u_long bc, cbc;
		
				src_y += count - 1;
				dest_y += count - 1;

				bc = count >> 4;
				count &= 0xf;
		
				while (bc--) {
					*dest_y-- = *src_y--; *dest_y-- = *src_y--;
					*dest_y-- = *src_y--; *dest_y-- = *src_y--;
					*dest_y-- = *src_y--; *dest_y-- = *src_y--;
					*dest_y-- = *src_y--; *dest_y-- = *src_y--;
					*dest_y-- = *src_y--; *dest_y-- = *src_y--;
					*dest_y-- = *src_y--; *dest_y-- = *src_y--;
					*dest_y-- = *src_y--; *dest_y-- = *src_y--;
					*dest_y-- = *src_y--; *dest_y-- = *src_y--;
				}
				while (count--) {
					*dest_y-- = *src_y--;
				}

				cbc = clr_count >> 4;
				clr_count &= 0xf;

				while (cbc--) {
					*clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
					*clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
					*clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
					*clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
				}
				while (clr_count--) {
					*clr_y++ = 0;
				}
			}
		}
	} else if (dy < 0) {
		int i;
		for (i=0; i < depth && mask; i++, mask >>= 1) {
			if (0x1 & mask) {
				u_long *pl = (u_long *)bm->plane[i];
				u_long *src_y = pl + (lwpr*(y-dy));
				u_long *dest_y = pl + (lwpr*y); 
				long count = lwpr*(height + dy);
				u_long *clr_y = dest_y + count;
				u_long clr_count = src_y - dest_y;
				u_long bc, cbc;

				bc = count >> 4;
				count &= 0xf;

				while (bc--) {
					*dest_y++ = *src_y++; *dest_y++ = *src_y++;
					*dest_y++ = *src_y++; *dest_y++ = *src_y++;
					*dest_y++ = *src_y++; *dest_y++ = *src_y++;
					*dest_y++ = *src_y++; *dest_y++ = *src_y++;
					*dest_y++ = *src_y++; *dest_y++ = *src_y++;
					*dest_y++ = *src_y++; *dest_y++ = *src_y++;
					*dest_y++ = *src_y++; *dest_y++ = *src_y++;
					*dest_y++ = *src_y++; *dest_y++ = *src_y++;
				}
				while (count--) {
					*dest_y++ = *src_y++;
				}

				cbc = clr_count >> 4;
				clr_count &= 0xf;

				while (cbc--) {
					*clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
					*clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
					*clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
					*clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
				}
				while (clr_count--) {
					*clr_y++ = 0;
				}
			}
		}
	}
}
#endif /* 0 */

static void
txrascpy (src_dst, copy, mode)
	unsigned short src_dst;
	short copy;
	signed short mode;
{
	unsigned int oldint;
	unsigned short crtc_r21_before = CRTC.r21;
	unsigned short add;

	add = (mode < 0) ? 0xfeff : 0x0101;

	CRTC.r21 = (mode & 0x0f) | 0x0100;	/* specify same time write mode & page */
	/*mfp.ddr = 0;*/			/* port is input */

	/*oldint = splhigh();*/			/* interrupt disable */
	while(--copy >= 0) {
		while(mfp.gpip & MFP_GPIP_HSYNC) /* wait for hsync */
			asm("nop");
		while(!(mfp.gpip & MFP_GPIP_HSYNC))
			asm("nop");
		CRTC.r22 = src_dst;		/* specify raster number */
		CRTC.crtctrl = 8;		/* raster copy start */

		src_dst += add;
	}
	/*splx(oldint);*/			/* interrupt enable */

	while(mfp.gpip & MFP_GPIP_HSYNC)	/* wait for hsync */
		asm("nop");
	while(!(mfp.gpip & MFP_GPIP_HSYNC))
		asm("nop");
	CRTC.crtctrl = 0;			/* raster copy end */

	/*crtc.r21 &= ~0x100;	*/		/* same time write off */
	CRTC.r21 = crtc_r21_before;
}

#endif
