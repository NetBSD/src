/*	$NetBSD: video_subr.c,v 1.2 2000/05/22 17:17:44 uch Exp $	*/

/*-
 * Copyright (c) 2000 UCHIYAMA Yasushi.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <arch/hpcmips/dev/video_subr.h>

#define BPP2 ({								\
	u_int8_t bitmap;						\
	bitmap = *(volatile u_int8_t*)addr;				\
	*(volatile u_int8_t*)addr =					\
		(bitmap & ~(0x3 << ((3 - (x % 4)) * 2)));		\
})

#define BPP4 ({								\
	u_int8_t bitmap;						\
	bitmap = *(volatile u_int8_t*)addr;				\
	*(volatile u_int8_t*)addr =					\
		(bitmap & ~(0xf << ((1 - (x % 2)) * 4)));		\
})

#define BPP8 ({								\
	*(volatile u_int8_t*)addr = 0xff;				\
})

#define BRESENHAM(a, b, c, d, func) ({					\
	u_int32_t fbaddr = vc->vc_fbvaddr;				\
	u_int32_t fbwidth = vc->vc_fbwidth;				\
	u_int32_t fbdepth = vc->vc_fbdepth;				\
	len = a, step = b -1;						\
	if (step == 0)							\
		return;							\
	kstep = len == 0 ? 0 : 1;					\
	for (i = k = 0, j = step / 2; i <= step; i++) {			\
		x = xbase c;						\
		y = ybase d;						\
		addr = fbaddr + (((y * fbwidth + x) * fbdepth) >> 3);	\
		func;							\
		j -= len;						\
		while (j < 0) {						\
			j += step;					\
			k += kstep;					\
		}							\
	}								\
})

#define DRAWLINE(func) ({						\
	if (x < 0) {							\
		if (y < 0) {						\
			if (_y < _x) {					\
				BRESENHAM(_y, _x, -i, -k, func);	\
			} else {					\
				BRESENHAM(_x, _y, -k, -i, func);	\
			}						\
		} else {						\
			if (_y < _x) {					\
				BRESENHAM(_y, _x, -i, +k, func);	\
			} else {					\
				BRESENHAM(_x, _y, -k, +i, func);	\
			}						\
		}							\
	} else {							\
		if (y < 0) {						\
			if (_y < _x) {					\
				BRESENHAM(_y, _x, +i, -k, func);	\
			} else {					\
				BRESENHAM(_x, _y, +k, -i, func);	\
			}						\
		} else {						\
			if (_y < _x) {					\
				BRESENHAM(_y, _x, +i, +k, func);	\
			} else {					\
				BRESENHAM(_x, _y, +k, +i, func);	\
			}						\
		}							\
	}								\
})

#define LINEFUNC(b)							\
static void linebpp##b __P((struct video_chip *, int, int, int, int));	\
static void								\
linebpp##b##(vc, x0, y0, x1, y1)					\
	struct video_chip *vc;						\
	int x0, y0, x1, y1;						\
{									\
	u_int32_t addr;							\
	int i, j, k, len, step, kstep;					\
	int x, _x, y, _y;						\
	int xbase, ybase;						\
	x = x1 - x0;							\
	y = y1 - y0;							\
	_x = abs(x);							\
	_y = abs(y);							\
	xbase = x0;							\
	ybase = y0;							\
	DRAWLINE(BPP##b##);						\
}

#define DOTFUNC(b)							\
static void dotbpp##b __P((struct video_chip *, int, int));		\
static void								\
dotbpp##b##(vc, x, y)							\
	struct video_chip *vc;						\
	int x, y;							\
{									\
	u_int32_t addr;							\
	addr = vc->vc_fbvaddr + (((y * vc->vc_fbwidth + x) *		\
				 vc->vc_fbdepth) >> 3);			\
	BPP##b;								\
}

LINEFUNC(2)
LINEFUNC(4)
LINEFUNC(8)
DOTFUNC(2)
DOTFUNC(4)
DOTFUNC(8)
static void linebpp_unimpl __P((struct video_chip *, int, int, int, int));
static void dotbpp_unimpl __P((struct video_chip *, int, int));

int
cmap_work_alloc(r, g, b, rgb, cnt)
	u_int8_t **r, **g, **b;
	u_int32_t **rgb;
	int cnt;
{
	KASSERT(r && g && b && rgb && LEGAL_CLUT_INDEX(cnt - 1));

	*r = malloc(cnt * sizeof(u_int8_t), M_DEVBUF, M_WAITOK);
	*g = malloc(cnt * sizeof(u_int8_t), M_DEVBUF, M_WAITOK);
	*b = malloc(cnt * sizeof(u_int8_t), M_DEVBUF, M_WAITOK);
	*rgb = malloc(cnt * sizeof(u_int32_t), M_DEVBUF, M_WAITOK);	

	return (!(*r && *g && *b && *rgb));
}

void
cmap_work_free(r, g, b, rgb)
	u_int8_t *r, *g, *b;
	u_int32_t *rgb;
{
	if (r)
		free(r, M_DEVBUF);
	if (g)
		free(g, M_DEVBUF);
	if (b)
		free(b, M_DEVBUF);
	if (rgb)
		free(rgb, M_DEVBUF);
}

void
rgb24_compose(rgb24, r, g, b, cnt)
	u_int32_t *rgb24;
	u_int8_t *r, *g, *b;
	int cnt;
{
	int i;
	KASSERT(rgb24 && r && g && b && LEGAL_CLUT_INDEX(cnt - 1));

	for (i = 0; i < cnt; i++) {
		*rgb24++ = RGB24(r[i], g[i], b[i]);
	}
}

void
rgb24_decompose(rgb24, r, g, b, cnt)
	u_int32_t *rgb24;
	u_int8_t *r, *g, *b;
	int cnt;
{
	int i;
	KASSERT(rgb24 && r && g && b && LEGAL_CLUT_INDEX(cnt - 1));

	for (i = 0; i < cnt; i++) {
		u_int32_t rgb = *rgb24++;
		*r++ = (rgb >> 16) & 0xff;
		*g++ = (rgb >> 8) & 0xff;
		*b++ = rgb & 0xff;
	}
}

/*
 * Debug routines.
 */
void
video_calibration_pattern(vc)
	struct video_chip *vc;
{
	int x, y;

	x = vc->vc_fbwidth - 40;
	y = vc->vc_fbheight - 40;
	video_line(vc, 40, 40, x , 40);
	video_line(vc, x , 40, x , y );
	video_line(vc, x , y , 40, y );
	video_line(vc, 40, y , 40, 40);
	video_line(vc, 40, 40, x , y );
	video_line(vc, x,  40, 40, y );
}

static void
linebpp_unimpl(vc, x0, y0, x1, y1)
	struct video_chip *vc;
	int x0, y0, x1, y1;
{
	return;
}

static void
dotbpp_unimpl(vc, x, y)
	struct video_chip *vc;
	int x, y;
{
	return;
}

void
video_attach_drawfunc(vc)
	struct video_chip *vc;
{
	switch (vc->vc_fbdepth) {
	default:
		vc->vc_drawline = linebpp_unimpl;
		vc->vc_drawdot = dotbpp_unimpl;
		break;
	case 8:
		vc->vc_drawline = linebpp8;
		vc->vc_drawdot = dotbpp8;
		break;
	case 4:
		vc->vc_drawline = linebpp4;
		vc->vc_drawdot = dotbpp4;
		break;
	case 2:
		vc->vc_drawline = linebpp2;
		vc->vc_drawdot = dotbpp2;
		break;
	}
}

void
video_line(vc, x0, y0, x1, y1)
	struct video_chip *vc;
	int x0, y0, x1, y1;
{
	if (vc->vc_drawline)
		vc->vc_drawline(vc, x0, y0, x1, y1);
}

void
video_dot(vc, x, y)
	struct video_chip *vc;
	int x, y;
{
	if (vc->vc_drawdot)
		vc->vc_drawdot(vc, x, y);
}
