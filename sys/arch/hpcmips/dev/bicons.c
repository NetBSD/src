/*	$NetBSD: bicons.c,v 1.3 1999/11/21 06:53:21 uch Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */
#include "opt_vr41x1.h"

#define HALF_FONT

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <dev/cons.h>

#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/stdarg.h>

#include <hpcmips/dev/biconsvar.h>
#include <hpcmips/dev/bicons.h>

#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrkiuvar.h>

extern unsigned char font_clR8x8_data[];
extern unsigned char font_clB8x8_data[];
#define FONT_HEIGHT	8
#define FONT_WIDTH	1

#if 0
volatile u_short *trace_mem = (u_short*)0xAA000000;
#define TRACE(a, b)  (trace_mem[80*(b) + (a)*4 + 4] = \
  ((b)%4 == 0?0x00f0:((b)%4 == 1?0x00ff:((b)%4 == 2?0xf0ff:0xffff))))
#else
#define TRACE(a, b)
#endif

#if 0
static void xmemset(volatile void *dst0, int c0, int length);
static void xmemcpy(volatile void *dst0, volatile const void *src0, int length);
#endif

static void put_oxel_D2_M2L_3(u_char*, u_char, u_char);
static void put_oxel_D2_M2L_3x2(u_char*, u_char, u_char);
static void put_oxel_D2_M2L_0(u_char*, u_char, u_char);
static void put_oxel_D2_M2L_0x2(u_char*, u_char, u_char);
static void put_oxel_D8_00(u_char*, u_char, u_char);
static void put_oxel_D8_FF(u_char*, u_char, u_char);
static void put_oxel_D16_0000(u_char*, u_char, u_char);
static void put_oxel_D16_FFFF(u_char*, u_char, u_char);

struct {
	int type;
	char *name;
	void (*func)(u_char*, u_char, u_char);
	u_char clear_byte;
	short oxel_bytes;
} fb_table[] = {
	{ BIFB_D2_M2L_3,	BIFBN_D2_M2L_3,		put_oxel_D2_M2L_3,	0,2 },
	{ BIFB_D2_M2L_3x2,	BIFBN_D2_M2L_3x2,	put_oxel_D2_M2L_3x2,	0,1 },
	{ BIFB_D2_M2L_0,	BIFBN_D2_M2L_0,		put_oxel_D2_M2L_0,0xff,	2 },
	{ BIFB_D2_M2L_0x2,	BIFBN_D2_M2L_0x2,	put_oxel_D2_M2L_0x2,0xff,1 },
	{ BIFB_D8_00,		BIFBN_D8_00,		put_oxel_D8_00,	0xff,	8 },
	{ BIFB_D8_FF,		BIFBN_D8_FF,		put_oxel_D8_FF,	0x00,	8 },
	{ BIFB_D16_0000,	BIFBN_D16_0000,		put_oxel_D16_0000,0xff,	16 },
	{ BIFB_D16_FFFF,	BIFBN_D16_FFFF,		put_oxel_D16_FFFF,0x00,	16 },
};
#define FB_TABLE_SIZE (sizeof(fb_table)/sizeof(*fb_table))


static u_char	*fb_vram	= 0;
static short	fb_line_bytes	= 0x50;
static u_char	fb_clear_byte	= 0;
short	bicons_ypixel	= 240;
short	bicons_xpixel	= 320;
#ifdef HALF_FONT
static short	fb_oxel_bytes	= 1;
short	bicons_width	= 80;
void	(*fb_put_oxel)(u_char*, u_char, u_char) = put_oxel_D2_M2L_3x2;
#else
static short	fb_oxel_bytes	= 2;
short	bicons_width	= 40;
void	(*fb_put_oxel)(u_char*, u_char, u_char) = put_oxel_D2_M2L_3;
#endif
short bicons_height	= 30;
static short curs_x	= 0;
static short curs_y	= 0;

static int	bicons_getc		__P((dev_t));
static void	bicons_putc		__P((dev_t, int));
static void	bicons_pollc	__P((dev_t, int));
static void	draw_char(int x, int y, int c);
static void	clear(int y, int height);
static void	scroll(int y, int height, int d);

struct consdev builtincd = {
	(void (*)(struct consdev *))0,	/* probe */
	(void (*)(struct consdev *))0,	/* init */
	(int  (*)(dev_t))     bicons_getc,	/* getc */
	(void (*)(dev_t, int))bicons_putc,	/* putc */
	(void (*)(dev_t, int))bicons_pollc,	/* pollc */
	makedev (0, 0),
	CN_DEAD,
};

static void
draw_char(int x, int y, int c)
{
	int i;
	unsigned char* p;
	
	if (!fb_vram) {
		return;
	}

	p = &fb_vram[(y * FONT_HEIGHT * fb_line_bytes) +
		    x * FONT_WIDTH * fb_oxel_bytes];
	for (i = 0; i < FONT_HEIGHT; i++) {
		(*fb_put_oxel)(p, font_clR8x8_data[FONT_WIDTH * (FONT_HEIGHT * c + i)],
			       0xff);
		p += (fb_line_bytes);
	}
}

static void
clear(int y, int height)
{
	unsigned char *p;

	if (!fb_vram) {
		return;
	}
	TRACE(1, 2);
	p = &fb_vram[y * fb_line_bytes];
	TRACE(1, 3);
	while (0 < height--) {
		TRACE(1, 4);
		memset(p, fb_clear_byte, bicons_width * fb_oxel_bytes * FONT_WIDTH);
		TRACE(1, 5);
		p += fb_line_bytes;
		TRACE(1, 6);
	}
	TRACE(1, 7);
}

static void
scroll(int y, int height, int d)
{
	unsigned char *from, *to;

	if (!fb_vram) {
		return;
	}
	if (d < 0) {
		from = &fb_vram[y * fb_line_bytes];
		to = from + d * fb_line_bytes;
		while (0 < height--) {
			memcpy(to, from, bicons_width * fb_oxel_bytes);
			from += fb_line_bytes;
			to += fb_line_bytes;
		}
	} else {
		from = &fb_vram[(y + height - 1) * fb_line_bytes];
		to = from + d * fb_line_bytes;
		while (0 < height--) {
			memcpy(to, from, bicons_xpixel * fb_oxel_bytes / 8);
			from -= fb_line_bytes;
			to -= fb_line_bytes;
		}
	}
}

void
bicons_puts(char *s)
{
	TRACE(2, 0);
	while (*s) {
		TRACE(2, 1);
		bicons_putc(NULL, *s++);
		TRACE(2, 2);
	}
	TRACE(2, 3);
}


void
bicons_putn(char *s, int n)
{
	while (0 < n--) {
		bicons_putc(NULL, *s++);
	}
}

void
#ifdef __STDC__
bicons_printf(const char *fmt, ...)
#else
bicons_printf(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
	char buf[1000];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	bicons_puts(buf);
}

int
bicons_getc(dev_t dev)
{
#ifdef VR41X1
	return vrkiu_getc();
#endif
	return 0;
}

static void
bicons_putc(dev_t dev, int c)
{
	int line_feed = 0;

	TRACE(2, 4);

	switch (c) {
	case 0x08: /* back space */
		if (--curs_x < 0) {
			curs_x = 0;
		}
		/* erase character ar cursor position */
		draw_char(curs_x, curs_y, ' ');
		break;
	case '\r':
		curs_x = 0;
		break;
	case '\n':
		curs_x = 0;
		line_feed = 1;
		break;
	default:
		TRACE(2, 5);
		draw_char(curs_x, curs_y, c);
		TRACE(2, 6);
		if (bicons_width <= ++curs_x) {
			TRACE(2, 7);
			curs_x = 0;
			line_feed = 1;
		}
	}

	TRACE(2, 8);
	if (line_feed) {
		TRACE(2, 9);
		if (bicons_height <= ++curs_y) {
			TRACE(2, 10);
			/* scroll up */
			scroll(FONT_HEIGHT, (bicons_height - 1) * FONT_HEIGHT, -FONT_HEIGHT);
			clear((bicons_height - 1) * FONT_HEIGHT, FONT_HEIGHT);
			curs_y--;
		}
	}
	TRACE(2, 11);
}

void
bicons_pollc(dev_t dev, int c)
{
	return;
}

void
bicons_init()
{
	int fb_index = -1; 
	TRACE(1, 0);

	if (bootinfo) { 
		TRACE(1, 1);
		for (fb_index = 0; fb_index < FB_TABLE_SIZE; fb_index++) {
			if (fb_table[fb_index].type == bootinfo->fb_type) {
				break;
			}
		}
		if (FB_TABLE_SIZE <= fb_index) {
			/*
			 *  Unknown frame buffer type, but what can I do ?
			 */
			fb_index = 0;
		}
		fb_vram = (unsigned char*)bootinfo->fb_addr;
		fb_line_bytes = bootinfo->fb_line_bytes;
		bicons_xpixel = bootinfo->fb_width;
		bicons_ypixel = bootinfo->fb_height;

		fb_put_oxel = fb_table[fb_index].func;
		fb_clear_byte = fb_table[fb_index].clear_byte;
		fb_oxel_bytes = fb_table[fb_index].oxel_bytes;
	} else {
		fb_vram = (unsigned char*)0xAA000000;
		fb_line_bytes = 0x50;
		fb_clear_byte = 0;
		bicons_ypixel = 240;
#ifdef HALF_FONT
		TRACE(1, 2);
		bicons_xpixel = 640;
		fb_oxel_bytes = 1;
		fb_put_oxel = put_oxel_D2_M2L_3x2;
#else
		TRACE(1, 3);
		bicons_xpixel = 320;
		fb_oxel_bytes = 2;
		fb_put_oxel = put_oxel_D2_M2L_3;
#endif
	}

	bicons_width = bicons_xpixel / (8 * FONT_WIDTH);
	bicons_height = bicons_ypixel / FONT_HEIGHT;
	clear(0, bicons_ypixel);

	curs_x = 0;
	curs_y = 0;

	bicons_puts("builtin console type = ");
	if (bootinfo) { 
		bicons_puts(fb_table[fb_index].name);
	} else {
#ifdef HALF_FONT
		bicons_puts("default(D2_M2L_3x2)");
#else
		bicons_puts("default(D2_M2L_3)");
#endif
	}
	bicons_puts("\n");

	TRACE(1, 4);
}

#if 0
void
xmemset(volatile void *dst0, int c0, int length)
{
	volatile unsigned char *dst = dst0;
  
	while (length != 0) {
		*dst++ = c0;
		--length;
	}
}

void
xmemcpy(volatile void *dst0, volatile const void *src0, int length)
{
	volatile char *dst = dst0;
	volatile const char *src = src0;

	while (length != 0) {
		*dst++ = *src++;
		--length;
	}
}
#endif

/*=============================================================================
 *
 *	D2_M2L_3
 *
 */
static void
put_oxel_D2_M2L_3(u_char* xaddr, u_char data, u_char mask)
{
#if 1
	u_short* addr = (u_short*)xaddr;
	static u_short map0[] = {
		0x0000, 0x0300, 0x0c00, 0x0f00, 0x3000, 0x3300, 0x3c00, 0x3f00,
		0xc000, 0xc300, 0xcc00, 0xcf00, 0xf000, 0xf300, 0xfc00, 0xff00,
	};
	static u_short map1[] = {
		0x0000, 0x0003, 0x000c, 0x000f, 0x0030, 0x0033, 0x003c, 0x003f,
		0x00c0, 0x00c3, 0x00cc, 0x00cf, 0x00f0, 0x00f3, 0x00fc, 0x00ff,
	};
	*addr = (map1[data >> 4] | map0[data & 0x0f]);
#else
	static unsigned char map[] = {
		0x00, 0x03, 0x0c, 0x0f, 0x30, 0x33, 0x3c, 0x3f,
		0xc0, 0xc3, 0xcc, 0xcf, 0xf0, 0xf3, 0xfc, 0xff,
	};
	u_char* addr = xaddr;

	*addr++ = (map[(data >> 4) & 0x0f] & map[(mask >> 4) & 0x0f]) |
		(*addr & ~map[(mask >> 4) & 0x0f]);
	*addr   = (map[(data >> 0) & 0x0f] & map[(mask >> 0) & 0x0f]) |
		(*addr & ~map[(mask >> 0) & 0x0f]);
#endif
}

/*=============================================================================
 *
 *	D2_M2L_3x2
 *
 */
static void
put_oxel_D2_M2L_3x2(u_char* xaddr, u_char data, u_char mask)
{
	register u_char odd = (data & 0xaa);
	register u_char even = (data & 0x55);

	*xaddr = (odd | (even << 1)) | ((odd >> 1) & even);
}

/*=============================================================================
 *
 *	D2_M2L_0
 *
 */
static void
put_oxel_D2_M2L_0(u_char* xaddr, u_char data, u_char mask)
{
#if 1
	u_short* addr = (u_short*)xaddr;
	static u_short map0[] = {
		0xff00, 0xfc00, 0xf300, 0xf000, 0xcf00, 0xcc00, 0xc300, 0xc000,
		0x3f00, 0x3c00, 0x3300, 0x3000, 0x0f00, 0x0c00, 0x0300, 0x0000,
	};
	static u_short map1[] = {
		0x00ff, 0x00fc, 0x00f3, 0x00f0, 0x00cf, 0x00cc, 0x00c3, 0x00c0,
		0x003f, 0x003c, 0x0033, 0x0030, 0x000f, 0x000c, 0x0003, 0x0000,
	};
	*addr = (map1[data >> 4] | map0[data & 0x0f]);
#else
	static unsigned char map[] = {
		0x00, 0x03, 0x0c, 0x0f, 0x30, 0x33, 0x3c, 0x3f,
		0xc0, 0xc3, 0xcc, 0xcf, 0xf0, 0xf3, 0xfc, 0xff,
	};
	u_char* addr = xaddr;

	*addr++ = (~(map[(data >> 4) & 0x0f] & map[(mask >> 4) & 0x0f])) |
		(*addr & ~map[(mask >> 4) & 0x0f]);
	*addr   = (~(map[(data >> 0) & 0x0f] & map[(mask >> 0) & 0x0f])) |
		(*addr & ~map[(mask >> 0) & 0x0f]);
#endif
}

/*=============================================================================
 *
 *	D2_M2L_0x2
 *
 */
static void
put_oxel_D2_M2L_0x2(u_char* xaddr, u_char data, u_char mask)
{
	register u_char odd = (data & 0xaa);
	register u_char even = (data & 0x55);

	*xaddr = ~((odd | (even << 1)) | ((odd >> 1) & even));
}

/*=============================================================================
 *
 *	D8_00
 *
 */
static void
put_oxel_D8_00(u_char* xaddr, u_char data, u_char mask)
{
	int i;
	u_char* addr = xaddr;

	for (i = 0; i < 8; i++) {
		if (mask & 0x80) {
			*addr = (data & 0x80) ? 0x00 : 0xFF;
		}
		addr++;
		data <<= 1;
		mask <<= 1;
	}
}

/*=============================================================================
 *
 *	D8_FF
 *
 */
static void
put_oxel_D8_FF(u_char* xaddr, u_char data, u_char mask)
{
	int i;
	u_char* addr = xaddr;

	for (i = 0; i < 8; i++) {
		if (mask & 0x80) {
			*addr = (data & 0x80) ? 0xFF : 0x00;
		}
		addr++;
		data <<= 1;
		mask <<= 1;
	}
}

/*=============================================================================
 *
 *	D16_0000
 *
 */
static void
put_oxel_D16_0000(u_char* xaddr, u_char data, u_char mask)
{
	int i;
	u_short* addr = (u_short*)xaddr;

	for (i = 0; i < 8; i++) {
		if (mask & 0x80) {
			*addr = (data & 0x80) ? 0x0000 : 0xFFFF;
		}
		addr++;
		data <<= 1;
		mask <<= 1;
	}
}

/*=============================================================================
 *
 *	D16_FFFF
 *
 */
static void
put_oxel_D16_FFFF(u_char* xaddr, u_char data, u_char mask)
{
	int i;
	u_short* addr = (u_short*)xaddr;

	for (i = 0; i < 8; i++) {
		if (mask & 0x80) {
			*addr = (data & 0x80) ? 0xFFFF : 0x0000;
		}
		addr++;
		data <<= 1;
		mask <<= 1;
	}
}
