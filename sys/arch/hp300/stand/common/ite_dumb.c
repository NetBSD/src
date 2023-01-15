/*	$NetBSD: ite_dumb.c,v 1.2 2023/01/15 06:19:46 tsutsui Exp $	*/

/*-
 * Copyright (c) 2011 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Mark Davies of the Department of Computer
 * Science, Victoria University of Wellington, New Zealand.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *
 * from: Utah $Hdr: ite_subr.c 1.2 92/01/20$
 *
 *	@(#)ite_subr.c  8.1 (Berkeley) 6/10/93
 */

#ifdef ITECONSOLE

#include <sys/param.h>

#include <hp300/stand/common/samachdep.h>
#include <hp300/stand/common/itereg.h>
#include <hp300/stand/common/itevar.h>

static void dumb_fontinit(struct ite_data *ip);
static void dumb_writeglyph(struct ite_data *, uint8_t *fbmem, uint8_t *);
static void dumb_flip_cursor(struct ite_data *ip);
static void dumb_windowmove(struct ite_data *, int, int, int, int, int, int,
    int);

static uint8_t *font;

void
dumb_init(struct ite_data *ip)
{

	ip->bmv = dumb_windowmove;	/* unused */
	ite_fontinfo(ip);
	dumb_clear(ip, 0, 0, ip->rows, ip->cols);
	dumb_fontinit(ip);
}

static void
dumb_fontinit(struct ite_data *ip)
{
	int bytewidth = (((ip->ftwidth - 1) / 8) + 1);
	int glyphsize = bytewidth * ip->ftheight;
	uint8_t fontbuf[500];
	uint8_t *dp, *fontp;
	int c, i, romp;

	/*
	 * We don't bother to copy font glyph into fbmem
	 * since there is no hardware windowmove ops.
	 * Prepare font data into contiguous memory instead.
	 */
	font = alloc((ip->ftwidth * ip->ftheight) * 128);
	romp = getword(ip, getword(ip, FONTROM) + FONTADDR) + FONTDATA;

	for (c = 0; c < 128; c++) {
		/* get font glyph from FONTROM */
		dp = fontbuf;
		for (i = 0; i < glyphsize; i++) {
			*dp++ = getbyte(ip, romp);
			romp += 2;
		}

		/* setup font data for 8bpp fbmem */
		fontp = &font[(ip->ftwidth * ip->ftheight) * c];
		dumb_writeglyph(ip, fontp, fontbuf);
	}
}

static void
dumb_writeglyph(struct ite_data *ip, uint8_t *fontmem, uint8_t *glyphp)
{
	int bn;
	int b, l;

	for (l = 0; l < ip->ftheight; l++) {
		bn = 7;
		for (b = 0; b < ip->ftwidth; b++) {
			if ((1 << bn) & *glyphp)
				*fontmem++ = 0x01;
			else
				*fontmem++ = 0x00;
			if (--bn < 0) {
				bn = 7;
				glyphp++;
			}
		}
		if (bn < 7)
			glyphp++;
	}
}

static void
dumb_flip_cursor(struct ite_data *ip)
{
	int l, w;
	uint8_t *pc;
	uint32_t *pc32;

	pc = (uint8_t *)ip->fbbase +
	    ((ip->ftheight * ip->cursory) * ip->fbwidth) +
	    ip->ftwidth * ip->cursorx;

	for (l = 0; l < ip->ftheight; l++) {
		/* assume (ip->ftwidth % sizeof(uint32_t) == 0) */
		pc32 = (uint32_t *)pc;
		for (w = 0; w < ip->ftwidth / sizeof(uint32_t); w++)
			pc32[w] ^= 0x01010101;
		pc += ip->fbwidth;
	}
}

void
dumb_cursor(struct ite_data *ip, int flag)
{

	switch (flag) {
	case MOVE_CURSOR:
		dumb_flip_cursor(ip);
		/* FALLTHROUGH */
	case DRAW_CURSOR:
		ip->cursorx = ip->curx;
		ip->cursory = ip->cury;
		/* FALLTHROUGH */
	default:
		dumb_flip_cursor(ip);
		break;
	}
}

void
dumb_putc(struct ite_data *ip, int c, int dy, int dx)
{
	int l, w;
	uint8_t *pc;
	uint32_t *pc32;
	uint32_t *fontp;

	pc = (uint8_t *)ip->fbbase +
	    ((ip->ftheight * ip->cursory) * ip->fbwidth) +
	    ip->ftwidth * ip->cursorx;
	fontp = (uint32_t *)&font[(ip->ftwidth * ip->ftheight) * c];

	for (l = 0; l < ip->ftheight; l++) {
		/* assume (ip->ftwidth % sizeof(uint32_t) == 0) */
		pc32 = (uint32_t *)pc;
		for (w = 0; w < ip->ftwidth / sizeof(uint32_t); w++)
			pc32[w] = *fontp++;
		pc += ip->fbwidth;
	}
}

void
dumb_clear(struct ite_data *ip, int sy, int sx, int h, int w)
{
	int l;
	uint8_t *pdst;

	pdst = (uint8_t *)ip->fbbase +
	    ((ip->ftheight * sy) * ip->fbwidth) + ip->ftwidth * sx;

	for (l = 0; l < ip->ftheight * h; l++) {
		/* slow, but just works */
		memset(pdst, 0, ip->ftwidth * w);
		pdst += ip->fbwidth;
	}
}

void
dumb_scroll(struct ite_data *ip)
{
	int l, h, w;
	uint8_t *psrc, *pdst;

	dumb_flip_cursor(ip);

	psrc = (uint8_t *)ip->fbbase + (ip->ftheight * ip->fbwidth);
	pdst = (uint8_t *)ip->fbbase;

	h = (ip->rows - 1) * ip->ftheight;
	w = ip->cols * ip->ftwidth;

	for (l = 0; l < h; l++) {
		/* slow, but just works and no scroll if no retry */
		memmove(pdst, psrc, w);
		psrc += ip->fbwidth;
		pdst += ip->fbwidth;
	}
}

static void
dumb_windowmove(struct ite_data *ip, int sy, int sx, int dy, int dx,
    int w, int h, int func)
{

	/* for sanity check */
	printf("%s: called\n", __func__);
}
#endif
