/*	$NetBSD: ite_tv.c,v 1.10 2003/07/15 01:44:52 lukem Exp $	*/

/*
 * Copyright (c) 1997 Masaru Oki.
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
 *    must display the following acknowledgement:
 *      This product includes software developed by Masaru Oki.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ite_tv.c,v 1.10 2003/07/15 01:44:52 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/grfioctl.h>

#include <arch/x68k/x68k/iodevice.h>
#include <arch/x68k/dev/itevar.h>
#include <arch/x68k/dev/grfvar.h>
#include <arch/x68k/dev/mfp.h>

/*
 * ITE device dependent routine for X680x0 Text-Video framebuffer.
 * Use X680x0 ROM fixed width font (8x16)
 */

#define CRTC    (IODEVbase->io_crtc)

/*
 * font constant
 */
#define FONTWIDTH   8
#define FONTHEIGHT  16
#define UNDERLINE   14

/*
 * framebuffer constant
 */
#define PLANEWIDTH  1024
#define PLANEHEIGHT 1024
#define PLANELINES  (PLANEHEIGHT / FONTHEIGHT)
#define ROWBYTES    (PLANEWIDTH  / FONTWIDTH)
#define PLANESIZE   (PLANEHEIGHT * ROWBYTES)

u_int  tv_top;
u_char *tv_row[PLANELINES];
char   *tv_font[256];
__volatile char *tv_kfont[0x7f];

u_char kern_font[256 * FONTHEIGHT];

#define PHYSLINE(y)  ((tv_top + (y)) % PLANELINES)
#define ROWOFFSET(y) ((y) * FONTHEIGHT * ROWBYTES)
#define CHADDR(y, x) (tv_row[PHYSLINE(y)] + (x))

#define SETGLYPH(to,from) memcpy(&kern_font[(from)*16],&kern_font[(to)*16], 16)
#define KFONTBASE(left)   ((left) * 32 * 0x5e - 0x21 * 32)

/* prototype */
void tv_init	__P((struct ite_softc *));
void tv_deinit	__P((struct ite_softc *));
void tv_putc	__P((struct ite_softc *, int, int, int, int));
void tv_cursor	__P((struct ite_softc *, int));
void tv_clear	__P((struct ite_softc *, int, int, int, int));
void tv_scroll	__P((struct ite_softc *, int, int, int, int));

__inline static int expbits __P((int));
__inline static void txrascpy __P((u_char, u_char, short, signed short));

static __inline void
txrascpy (src, dst, size, mode)
	u_char src, dst;
	short size;
	signed short mode;
{
	/*int s;*/
	u_short saved_r21 = CRTC.r21;
	char d;

	d = (mode < 0) ? -1 : 1;
	src *= FONTHEIGHT / 4;
	dst *= FONTHEIGHT / 4;
	size *= 4;
	if (d < 0) {
		src += (FONTHEIGHT / 4) - 1;
		dst += (FONTHEIGHT / 4) - 1;
	}

	/* specify same time write mode & page */
	CRTC.r21 = (mode & 0x0f) | 0x0100;
	/*mfp.ddr = 0;*/			/* port is input */

	/*s = splhigh();*/
	while (--size >= 0) {
		/* wait for hsync */
		mfp_wait_for_hsync ();
		CRTC.r22 = (src << 8) | dst;	/* specify raster number */
		/* start raster copy */
		CRTC.crtctrl = 8;

		src += d;
		dst += d;
	}
	/*splx(s);*/

	/* wait for hsync */
	mfp_wait_for_hsync ();

	/* stop raster copy */
	CRTC.crtctrl = 0;

	CRTC.r21 = saved_r21;
}

/*
 * Change glyphs from SRAM switch.
 */
void
ite_set_glyph(void)
{
	u_char glyph = IODEVbase->io_sram[0x59];
	
	if (glyph & 4)
		SETGLYPH(0x82, '|');
	if (glyph & 2)
		SETGLYPH(0x81, '~');
	if (glyph & 1)
		SETGLYPH(0x80, '\\');
}

/*
 * Initialize
 */
void
tv_init(ip)
	struct ite_softc *ip;
{
	short i;

	/*
	 * initialize private variables
	 */
	tv_top = 0;
	for (i = 0; i < PLANELINES; i++)
		tv_row[i] = (void *)&IODEVbase->tvram[ROWOFFSET(i)];
	/* shadow ANK font */
	memcpy(kern_font, (void *)&IODEVbase->cgrom0_8x16, 256 * FONTHEIGHT);
	ite_set_glyph();
	/* set font address cache */
	for (i = 0; i < 256; i++)
		tv_font[i] = &kern_font[i * FONTHEIGHT];
	for (i = 0x21; i < 0x30; i++)
		tv_kfont[i] = &IODEVbase->cgrom0_16x16[KFONTBASE(i-0x21)];
	for (; i < 0x50; i++)
		tv_kfont[i] = &IODEVbase->cgrom1_16x16[KFONTBASE(i-0x30)];
	for (; i < 0x7f; i++)
		tv_kfont[i] = &IODEVbase->cgrom2_16x16[KFONTBASE(i-0x50)];

	/*
	 * initialize part of ip
	 */
	ip->cols = ip->grf->g_display.gd_dwidth  / FONTWIDTH;
	ip->rows = ip->grf->g_display.gd_dheight / FONTHEIGHT;
	/* set draw routine dynamically */
	ip->isw->ite_putc   = tv_putc;
	ip->isw->ite_cursor = tv_cursor;
	ip->isw->ite_clear  = tv_clear;
	ip->isw->ite_scroll = tv_scroll;

	/*
	 * Intialize colormap
	 */
#define RED   (0x1f << 6)
#define BLUE  (0x1f << 1)
#define GREEN (0x1f << 11)
	IODEVbase->tpalet[0] = 0;			/* black */
	IODEVbase->tpalet[1] = 1 | RED;			/* red */
	IODEVbase->tpalet[2] = 1 | GREEN;		/* green */
	IODEVbase->tpalet[3] = 1 | RED | GREEN;		/* yellow */
	IODEVbase->tpalet[4] = 1 | BLUE;		/* blue */
	IODEVbase->tpalet[5] = 1 | BLUE | RED;		/* magenta */
	IODEVbase->tpalet[6] = 1 | BLUE | GREEN;	/* cyan */
	IODEVbase->tpalet[7] = 1 | BLUE | RED | GREEN;	/* white */
}

/*
 * Deinitialize
 */
void
tv_deinit(ip)
	struct ite_softc *ip;
{
	ip->flags &= ~ITE_INITED; /* XXX? */
}

typedef void tv_putcfunc __P((struct ite_softc *, int, char *));
static tv_putcfunc tv_putc_nm;
static tv_putcfunc tv_putc_in;
static tv_putcfunc tv_putc_ul;
static tv_putcfunc tv_putc_ul_in;
static tv_putcfunc tv_putc_bd;
static tv_putcfunc tv_putc_bd_in;
static tv_putcfunc tv_putc_bd_ul;
static tv_putcfunc tv_putc_bd_ul_in;

static tv_putcfunc *putc_func[ATTR_ALL + 1] = {
	tv_putc_nm,
	tv_putc_in,
	tv_putc_ul,
	tv_putc_ul_in,
	tv_putc_bd,
	tv_putc_bd_in,
	tv_putc_bd_ul,
	tv_putc_bd_ul_in,
	/* no support for blink */
	tv_putc_nm,
	tv_putc_in,
	tv_putc_ul,
	tv_putc_ul_in,
	tv_putc_bd,
	tv_putc_bd_in,
	tv_putc_bd_ul,
	tv_putc_bd_ul_in,
};

/*
 * simple put character function
 */
void
tv_putc(ip, ch, y, x, mode)
	struct ite_softc *ip;
	int ch, y, x, mode;
{
	char *p = CHADDR(y, x);
	short fh;

	/* multi page write mode */
	CRTC.r21 = 0x0100 | ip->fgcolor << 4;

	/* draw plane */
	putc_func[mode](ip, ch, p);

	/* erase plane */
	CRTC.r21 ^= 0x00f0;
	if (ip->save_char) {
		for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES)
			*(u_short *)p = 0;
	} else {
		for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES)
			*p = 0;
	}

	/* crtc mode reset */
	CRTC.r21 = 0;
}

void
tv_putc_nm(ip, ch, p)
	struct ite_softc *ip;
	int ch;
	char *p;
{
	short fh, hi;
	char *f;
	short *kf;

	hi = ip->save_char & 0x7f;

	if (hi >= 0x21 && hi <= 0x7e) {
		/* multibyte character */
		kf = (short *)tv_kfont[hi];
		kf += (ch & 0x7f) * FONTHEIGHT;
		/* draw plane */
		for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES)
			*(u_short *)p = *kf++;
		return;
	}

	/* singlebyte character */
	if (*ip->GL == CSET_JISKANA)
		ch |= 0x80;
	f = tv_font[ch];

	/* draw plane */
	for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES)
		*p = *f++;
}

void
tv_putc_in(ip, ch, p)
	struct ite_softc *ip;
	int ch;
	char *p;
{
	short fh, hi;
	char *f;
	short *kf;

	hi = ip->save_char & 0x7f;

	if (hi >= 0x21 && hi <= 0x7e) {
		/* multibyte character */
		kf = (short *)tv_kfont[hi];
		kf += (ch & 0x7f) * FONTHEIGHT;
		/* draw plane */
		for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES)
			*(u_short *)p = ~*kf++;
		return;
	}

	/* singlebyte character */
	if (*ip->GL == CSET_JISKANA)
		ch |= 0x80;
	f = tv_font[ch];

	/* draw plane */
	for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES)
		*p = ~*f++;
}

void
tv_putc_bd(ip, ch, p)
	struct ite_softc *ip;
	int ch;
	char *p;
{
	short fh, hi;
	char *f;
	short *kf;

	hi = ip->save_char & 0x7f;

	if (hi >= 0x21 && hi <= 0x7e) {
		/* multibyte character */
		kf = (short *)tv_kfont[hi];
		kf += (ch & 0x7f) * FONTHEIGHT;
		/* draw plane */
		for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES) {
			ch = *kf++;
			*(u_short *)p = ch | (ch >> 1);
		}
		return;
	}

	/* singlebyte character */
	if (*ip->GL == CSET_JISKANA)
		ch |= 0x80;
	f = tv_font[ch];

	/* draw plane */
	for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES) {
		ch = *f++;
		*p = ch | (ch >> 1);
	}
}

__inline static int
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

void
tv_putc_ul(ip, ch, p)
	struct ite_softc *ip;
	int ch;
	char *p;
{
	short fh, hi;
	char *f;
	short *kf;

	hi = ip->save_char & 0x7f;

	if (hi >= 0x21 && hi <= 0x7e) {
		/* multibyte character */
		kf = (short *)tv_kfont[hi];
		kf += (ch & 0x7f) * FONTHEIGHT;
		/* draw plane */
		for (fh = 0; fh < UNDERLINE; fh++, p += ROWBYTES)
			*(u_short *)p = *kf++;
		*(u_short *)p = expbits(*kf++);
		p += ROWBYTES;
		for (fh++; fh < FONTHEIGHT; fh++, p += ROWBYTES)
			*(u_short *)p = *kf++;
		return;
	}

	/* singlebyte character */
	if (*ip->GL == CSET_JISKANA)
		ch |= 0x80;
	f = tv_font[ch];

	/* draw plane */
	for (fh = 0; fh < UNDERLINE; fh++, p += ROWBYTES)
		*p = *f++;
	*p = expbits(*f++);
	p += ROWBYTES;
	for (fh++; fh < FONTHEIGHT; fh++, p += ROWBYTES)
		*p = *f++;
}

void
tv_putc_bd_in(ip, ch, p)
	struct ite_softc *ip;
	int ch;
	char *p;
{
	short fh, hi;
	char *f;
	short *kf;

	hi = ip->save_char & 0x7f;

	if (hi >= 0x21 && hi <= 0x7e) {
		/* multibyte character */
		kf = (short *)tv_kfont[hi];
		kf += (ch & 0x7f) * FONTHEIGHT;
		/* draw plane */
		for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES) {
			ch = *kf++;
			*(u_short *)p = ~(ch | (ch >> 1));
		}
		return;
	}

	/* singlebyte character */
	if (*ip->GL == CSET_JISKANA)
		ch |= 0x80;
	f = tv_font[ch];

	/* draw plane */
	for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES) {
		ch = *f++;
		*p = ~(ch | (ch >> 1));
	}
}

void
tv_putc_ul_in(ip, ch, p)
	struct ite_softc *ip;
	int ch;
	char *p;
{
	short fh, hi;
	char *f;
	short *kf;

	hi = ip->save_char & 0x7f;

	if (hi >= 0x21 && hi <= 0x7e) {
		/* multibyte character */
		kf = (short *)tv_kfont[hi];
		kf += (ch & 0x7f) * FONTHEIGHT;
		/* draw plane */
		for (fh = 0; fh < UNDERLINE; fh++, p += ROWBYTES)
			*(u_short *)p = ~*kf++;
		*(u_short *)p = ~expbits(*kf++);
		p += ROWBYTES;
		for (fh++; fh < FONTHEIGHT; fh++, p += ROWBYTES)
			*(u_short *)p = ~*kf++;
		return;
	}

	/* singlebyte character */
	if (*ip->GL == CSET_JISKANA)
		ch |= 0x80;
	f = tv_font[ch];

	/* draw plane */
	for (fh = 0; fh < UNDERLINE; fh++, p += ROWBYTES)
		*p = ~*f++;
	*p = ~expbits(*f++);
	p += ROWBYTES;
	for (fh++; fh < FONTHEIGHT; fh++, p += ROWBYTES)
		*p = ~*f++;
}

void
tv_putc_bd_ul(ip, ch, p)
	struct ite_softc *ip;
	int ch;
	char *p;
{
	short fh, hi;
	char *f;
	short *kf;

	hi = ip->save_char & 0x7f;

	if (hi >= 0x21 && hi <= 0x7e) {
		/* multibyte character */
		kf = (short *)tv_kfont[hi];
		kf += (ch & 0x7f) * FONTHEIGHT;
		/* draw plane */
		for (fh = 0; fh < UNDERLINE; fh++, p += ROWBYTES) {
			ch = *kf++;
			*(u_short *)p = ch | (ch >> 1);
		}
		ch = *kf++;
		*(u_short *)p = expbits(ch | (ch >> 1));
		p += ROWBYTES;
		for (fh++; fh < FONTHEIGHT; fh++, p += ROWBYTES) {
			ch = *kf++;
			*(u_short *)p = ch | (ch >> 1);
		}
		return;
	}

	/* singlebyte character */
	if (*ip->GL == CSET_JISKANA)
		ch |= 0x80;
	f = tv_font[ch];

	/* draw plane */
	for (fh = 0; fh < UNDERLINE; fh++, p += ROWBYTES) {
		ch = *f++;
		*p = ch | (ch >> 1);
	}
	ch = *f++;
	*p = expbits(ch | (ch >> 1));
	p += ROWBYTES;
	for (fh++; fh < FONTHEIGHT; fh++, p += ROWBYTES) {
		ch = *f++;
		*p = ch | (ch >> 1);
	}
}

void
tv_putc_bd_ul_in(ip, ch, p)
	struct ite_softc *ip;
	int ch;
	char *p;
{
	short fh, hi;
	char *f;
	short *kf;

	hi = ip->save_char & 0x7f;

	if (hi >= 0x21 && hi <= 0x7e) {
		/* multibyte character */
		kf = (short *)tv_kfont[hi];
		kf += (ch & 0x7f) * FONTHEIGHT;
		/* draw plane */
		for (fh = 0; fh < UNDERLINE; fh++, p += ROWBYTES) {
			ch = *kf++;
			*(u_short *)p = ~(ch | (ch >> 1));
		}
		ch = *kf++;
		*(u_short *)p = ~expbits(ch | (ch >> 1));
		p += ROWBYTES;
		for (fh++; fh < FONTHEIGHT; fh++, p += ROWBYTES) {
			ch = *kf++;
			*(u_short *)p = ~(ch | (ch >> 1));
		}
		return;
	}

	/* singlebyte character */
	if (*ip->GL == CSET_JISKANA)
		ch |= 0x80;
	f = tv_font[ch];

	/* draw plane */
	for (fh = 0; fh < UNDERLINE; fh++, p += ROWBYTES) {
		ch = *f++;
		*p = ~(ch | (ch >> 1));
	}
	ch = *f++;
	*p = ~expbits(ch | (ch >> 1));
	p += ROWBYTES;
	for (fh++; fh < FONTHEIGHT; fh++, p += ROWBYTES) {
		ch = *f++;
		ch |= ch >> 1;
		*p = ~(ch | (ch >> 1));
	}
}

/*
 * draw/erase/move cursor
 */
void
tv_cursor(ip, flag)
	struct ite_softc *ip;
	int flag;
{
	u_char *p;
	short fh;

	/* erase */
	switch (flag) {
	/*case DRAW_CURSOR:*/
	/*case ERASE_CURSOR:*/
	/*case MOVE_CURSOR:*/
	case START_CURSOROPT:
		/*
		 * old: ip->cursorx, ip->cursory
		 * new: ip->curx, ip->cury
		 */
		p = CHADDR(ip->cursory, ip->cursorx);
		for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES)
			*p = ~*p;
		break;
	}

	/* draw */
	switch (flag) {
	/*case MOVE_CURSOR:*/
	case END_CURSOROPT:
		/*
		 * Use exclusive-or.
		 */
		p = CHADDR(ip->cury, ip->curx);
		for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES)
			*p = ~*p;

		ip->cursorx = ip->curx;
		ip->cursory = ip->cury;
		break;
	}
}

/*
 * clear rectangle
 */
void
tv_clear(ip, y, x, height, width)
	struct ite_softc *ip;
	int y, x, height, width;
{
	char *p;
	short fh;

	/* XXX: reset scroll register on clearing whole screen */
	if (y == 0 && x == 0 && height == ip->rows && width == ip->cols) {
		CRTC.r10 = 0;
		CRTC.r11 = tv_top * FONTHEIGHT;
	}

	CRTC.r21 = 0x01f0;
	while (height--) {
		p = CHADDR(y++, x);
		for (fh = 0; fh < FONTHEIGHT; fh++, p += ROWBYTES)
			memset(p, 0, width);
	}
	/* crtc mode reset */
	CRTC.r21 = 0;
}

/*
 * scroll lines/columns
 */
void
tv_scroll(ip, srcy, srcx, count, dir)
	struct ite_softc *ip;
	int srcy, srcx, count, dir;
{
	int dst, siz, pl;

	switch (dir) {
	case SCROLL_UP:
		/*
		 * src: srcy
		 * dst: (srcy - count)
		 * siz: (ip->bottom_margin - sy + 1)
		 */
		dst = srcy - count;
		siz = ip->bottom_margin - srcy + 1;
		if (dst == 0 && ip->bottom_margin == ip->rows - 1) {
			/* special case, hardware scroll */
			tv_top = (tv_top + count) % PLANELINES;
			CRTC.r11 = tv_top * FONTHEIGHT;
		} else {
			srcy = PHYSLINE(srcy);
			dst = PHYSLINE(dst);
			txrascpy(srcy, dst, siz, 0x0f);
		}
		break;

	case SCROLL_DOWN:
		/*
		 * src: srcy
		 * dst: (srcy + count)
		 * siz: (ip->bottom_margin - dy + 1)
		 */
		dst = srcy + count;
		siz = ip->bottom_margin - dst + 1;
		if (srcy == 0 && ip->bottom_margin == ip->rows - 1) {
			/* special case, hardware scroll */
			tv_top = (tv_top + PLANELINES - count) % PLANELINES;
			CRTC.r11 = tv_top * FONTHEIGHT;
		} else {
			srcy = PHYSLINE(srcy) + siz - 1;
			dst = PHYSLINE(dst) + siz - 1;
			txrascpy(srcy, dst, siz, 0x0f | 0x8000);
		}
		break;

	case SCROLL_LEFT:
		for (pl = 0; pl < PLANESIZE * 4; pl += PLANESIZE) {
			short fh;
			char *src = CHADDR(srcy, srcx) + pl;
			char *dst = CHADDR(srcy, srcx - count) + pl;

			siz = ip->cols - srcx;
			for (fh = 0; fh < FONTHEIGHT; fh++) {
				memcpy(dst, src, siz);
				src += ROWBYTES;
				dst += ROWBYTES;
			}
		}
		break;

	case SCROLL_RIGHT:
		for (pl = 0; pl < PLANESIZE * 4; pl += PLANESIZE) {
			short fh;
			char *src = CHADDR(srcy, srcx) + pl;
			char *dst = CHADDR(srcy, srcx + count) + pl;

			siz = ip->cols - (srcx + count);
			for (fh = 0; fh < FONTHEIGHT; fh++) {
				memcpy(dst, src, siz);
				src += ROWBYTES;
				dst += ROWBYTES;
			}
		}
		break;
	}
}
