/*	$NetBSD: cons_fb.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <machine/sbd.h>

#include "console.h"

struct fb fb;

void
fb_set_addr(uint32_t fb_addr, uint32_t fb_size, uint32_t font_addr)
{

	fb.fb_addr = (uint8_t *)fb_addr;
	fb.fb_size = fb_size;
	fb.font_addr = (uint8_t *)font_addr;

	cons.init = fb_init;
	cons.putc = fb_drawchar;
	cons.scroll = fb_scroll;
	cons.cursor = fb_drawcursor;
}

void *
fb_get_addr(void)
{

	return fb.fb_addr;
}

void
fb_init(void)
{

	cons.x = X_INIT;
	cons.y = Y_INIT;
	fb.active = TRUE;
	fb_clear(0, 0, FB_WIDTH, FB_HEIGHT, CONS_BG);
}

void
fb_active(boolean_t on)
{

	if (fb.active && !on)
		printf("FB disabled.\n");

	fb.active = on;

	if (fb.active && on)
		printf("FB enabled.\n");
}

void
fb_scroll(void)
{

	if (!fb.active)
		return;
#if 0	/* 1-line scroll */
	cons.y--;
	fb_copy(0, ROM_FONT_HEIGHT, 0, 0,
	    FB_WIDTH, FB_HEIGHT * (CONS_HEIGHT - 1));
	fb_clear(0, cons.y * ROM_FONT_HEIGHT, FB_WIDTH, ROM_FONT_HEIGHT,
	    CONS_BG);
#else	/* jump scroll */
	cons.y /= 2;
	fb_copy(0, cons.y * ROM_FONT_HEIGHT, 0, 0, FB_WIDTH, FB_HEIGHT / 2);
	fb_clear(0, cons.y *ROM_FONT_HEIGHT, FB_WIDTH, FB_HEIGHT / 2, CONS_BG);
#endif
}

#define	MINMAX(x, min, max)						\
	((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
void
fb_clear(int x, int y, int w, int h, int q)
{
	uint8_t *p;
	int i, j, k, xend, yend;

	if (!fb.active)
		return;

	x = MINMAX(x, 0, FB_WIDTH);
	y = MINMAX(y, 0, FB_HEIGHT);
	xend = MINMAX(x + w, 0, FB_WIDTH);
	yend = MINMAX(y + h , 0, FB_HEIGHT);

	p = (uint8_t *)fb.fb_addr + x + y * FB_LINEBYTES;

	j = xend - x;
	k = j + FB_LINEBYTES - w;
	for (i = y; i < yend; i++, p+= k)
		memset(p, q, j);
}

void
fb_copy(int x0, int y0, int x1, int y1, int w, int h)
{
	int x1end, y1end, i, j, k;
	uint8_t *p, *q;

	if (!fb.active)
		return;

	x0 = MINMAX(x0, 0, FB_WIDTH);
	y0 = MINMAX(y0, 0, FB_HEIGHT);
	x1 = MINMAX(x1, 0, FB_WIDTH);
	y1 = MINMAX(y1, 0, FB_HEIGHT);
	x1end = MINMAX(x1 + w, 0, FB_WIDTH);
	y1end = MINMAX(y1 + h, 0, FB_HEIGHT);

	p = fb.fb_addr + x1 + y1 * FB_LINEBYTES;
	q = fb.fb_addr + x0 + y0 * FB_LINEBYTES;

	j = x1end - x1;
	k = j + FB_LINEBYTES - w;
	for (i = y1; i < y1end; i++, p += k, q += k)
		memmove(p, q, j);
}
#undef MINMAX

void
fb_drawchar(int x, int y, int c)
{
	uint16_t *font_addr;
	int font_ofs;

	if (!fb.active)
		return;

	if ((font_ofs = (c & 0x7f) - 0x20) < 0)
		return;

	font_addr = (uint16_t *)(fb.font_addr +
	    font_ofs * sizeof(uint16_t) * ROM_FONT_HEIGHT);

	fb_drawfont(x, y, font_addr);
}

void
fb_drawfont(int x, int y, uint16_t *font_addr)
{
	uint8_t *fb_addr;
	uint16_t bitmap;
	int i, j;

	if (!fb.active)
		return;

	fb_addr = fb.fb_addr + x + y * FB_LINEBYTES;

	for (i = 0;  i < 24; i++) {
		bitmap = *font_addr++;
		for (j = 0; j < 12; j++, bitmap <<= 1)
			fb_addr[j] = bitmap & 0x8000 ? CONS_FG : CONS_BG;
		fb_addr += FB_LINEBYTES;
	}
}

void
fb_drawcursor(int x, int y)
{
	uint8_t *fb_addr;
	int i, j;

	if (!fb.active)
		return;

	fb_addr = fb.fb_addr + x + y * FB_LINEBYTES;
	for (i = 0;  i < 24; i++) {
		for (j = 0; j < 12; j++)
			fb_addr[j] = fb_addr[j] == CONS_FG ? CONS_BG : CONS_FG;
		fb_addr += FB_LINEBYTES;
	}
}
