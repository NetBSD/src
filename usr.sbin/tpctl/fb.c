/*	$NetBSD: fb.c,v 1.2 2003/07/08 23:33:50 uwe Exp $	*/

/*-
 * Copyright (c) 2002 TAKEMRUA Shin
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

#include "tpctl.h"

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: fb.c,v 1.2 2003/07/08 23:33:50 uwe Exp $");
#endif /* not lint */

#define INVALID_CACHE -1
#define ALIGN(a, n)	((typeof(a))(((int)(a) + (n) - 1) / (n) * (n)))
#define ABS(a)		((a) < 0 ? -(a) : (a))
#define SWAP(a, b)	do {				\
		typeof(a) tmp;				\
		tmp = (a); (a) = (b); (b) = tmp;	\
	} while(0)
#define bitsizeof(t)	(sizeof(t) * 8)

int
fb_dispmode(struct fb *fb, int dispmode)
{

	if (fb->dispmode != dispmode) {
	    if (ioctl(fb->fd, WSDISPLAYIO_SMODE, &dispmode) < 0)
		return (-1);
	    fb->dispmode = dispmode;
	}

	return (0);
}

int
fb_init(struct fb *fb, int fd)
{
	int y;
	size_t size;

	fb->fd = fd;
	fb->linecache_y = INVALID_CACHE;
	fb->conf.hf_conf_index = HPCFB_CURRENT_CONFIG;
	if (ioctl(fb->fd, WSDISPLAYIO_GMODE, &fb->dispmode) < 0)
		return (-1);
	if (ioctl(fb->fd, HPCFBIO_GCONF, &fb->conf) < 0)
		return (-1);

	if (fb_dispmode(fb, WSDISPLAYIO_MODE_MAPPED) < 0)
		return (-1);

	size = (size_t)fb->conf.hf_bytes_per_line * fb->conf.hf_height;
	size += fb->conf.hf_offset;
	size = ALIGN(size, getpagesize());
	fb->baseaddr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
	if (fb->baseaddr == MAP_FAILED)
		return (-1);
	fb->baseaddr += fb->conf.hf_offset;

	size = ALIGN(fb->conf.hf_bytes_per_line, 16);
	fb->linecache = (fb_pixel_t*)malloc(size);
	if (fb->linecache == NULL)
		return (-1);
	fb->workbuf = (fb_pixel_t*)malloc(size);
	if (fb->workbuf == NULL)
		return (-1);

	if (fb->conf.hf_access_flags & HPCFB_ACCESS_REVERSE) {
		fb->white = 0;
		fb->black = ~0;
	} else {
		fb->white = ~0;
		fb->black = 0;
	}

	/*
	 * clear screen
	 */
	for (y = 0; y < fb->conf.hf_height; y++) {
		fb_getline(fb, y);
		memset(fb->linecache, fb->black,
		    ALIGN(fb->conf.hf_bytes_per_line, 16));
		fb_putline(fb, y);
	}

	return (0);
}

static void
__fb_swap_workbuf(struct fb *fb)
{
	int i, n;

	n = ALIGN(fb->conf.hf_bytes_per_line, 16) / sizeof(fb_pixel_t);
	if (fb->conf.hf_order_flags & HPCFB_REVORDER_BYTE) {
		for (i = 0; i < n; i++)
			fb->workbuf[i] = 
			    ((fb->workbuf[i] << 8) & 0xff00ff00) |
			    ((fb->workbuf[i] >> 8) & 0x00ff00ff);
	}
	if (fb->conf.hf_order_flags & HPCFB_REVORDER_WORD) {
		for (i = 0; i < n; i++)
			fb->workbuf[i] = 
			    ((fb->workbuf[i] << 16) & 0xffff0000) |
			    ((fb->workbuf[i] >> 16) & 0x0000ffff);
	}
	if (fb->conf.hf_order_flags & HPCFB_REVORDER_DWORD) {
		for (i = 0; i < n; i += 2) {
			fb_pixel_t tmp;
			tmp = fb->workbuf[i];
			fb->workbuf[i] = fb->workbuf[i + 1];
			fb->workbuf[i + 1] = tmp;
		}
	}
	if (fb->conf.hf_order_flags & HPCFB_REVORDER_QWORD) {
		for (i = 0; i < n; i += 4) {
			fb_pixel_t tmp;
			tmp = fb->workbuf[i + 0];
			fb->workbuf[i + 0] = fb->workbuf[i + 2];
			fb->workbuf[i + 2] = tmp;
			tmp = fb->workbuf[i + 1];
			fb->workbuf[i + 1] = fb->workbuf[i + 3];
			fb->workbuf[i + 3] = tmp;
		}
	}
}

static void
__fb_put_pixel(struct fb *fb, fb_pixel_t pixel, int width, int x)
{
	fb_pixel_t mask = (1 << width) - 1;

	x -= (bitsizeof(fb_pixel_t) - width);
	if (x < 0) {
		pixel <<= -x;
		mask <<= -x;
		fb->linecache[0] = (fb->linecache[0]&~mask) | (pixel&~mask);
	} else {
		fb_pixel_t *dst = &fb->linecache[x / bitsizeof(fb_pixel_t)];
		x %= bitsizeof(fb_pixel_t);
		*dst = (*dst & ~(mask>>x)) | ((pixel>>x) & (mask>>x));
		dst++;
		if (x == 0)
			return;
		x = bitsizeof(fb_pixel_t) - x;
		*dst = (*dst & ~(mask<<x)) | ((pixel<<x) & (mask<<x));
	}
}

void
fb_getline(struct fb *fb, int y)
{
	int i, n;
	unsigned char *src;
	fb_pixel_t *dst;

	src = fb->baseaddr + fb->conf.hf_bytes_per_line * y;
	dst = fb->workbuf;
	n = ALIGN(fb->conf.hf_bytes_per_line, 16) / sizeof(fb_pixel_t);
	for (i = 0; i < n; i++) {
		*dst++ = ((fb_pixel_t)src[0] << 24) | 
		    ((fb_pixel_t)src[1] << 16) | 
		    ((fb_pixel_t)src[2] << 8) | 
		    ((fb_pixel_t)src[3] << 0);
		src += 4;
	}

	__fb_swap_workbuf(fb);
	memcpy(fb->linecache, fb->workbuf, n * sizeof(fb_pixel_t));
}

void
fb_putline(struct fb *fb, int y)
{
	int i, n;
	unsigned char *dst;
	fb_pixel_t *src;

	src = fb->workbuf;
	dst = fb->baseaddr + fb->conf.hf_bytes_per_line * y;
	n = ALIGN(fb->conf.hf_bytes_per_line, 16) / sizeof(fb_pixel_t);
	memcpy(fb->workbuf, fb->linecache, n * sizeof(fb_pixel_t));
	__fb_swap_workbuf(fb);
	for (i = 0; i < n; i++) {
		*dst++ = (*src >> 24) & 0xff;
		*dst++ = (*src >> 16) & 0xff;
		*dst++ = (*src >>  8) & 0xff;
		*dst++ = (*src >>  0) & 0xff;
		src++;
	}
}

void
fb_fetchline(struct fb *fb, int y)
{
	if (fb->linecache_y == y)
		return;
	fb_getline(fb, y);
	fb->linecache_y = y;
}

void
fb_flush(struct fb *fb)
{
	if (fb->linecache_y != INVALID_CACHE)
		fb_putline(fb, fb->linecache_y);
}

void
fb_drawpixel(struct fb *fb, int x, int y, fb_pixel_t pixel)
{
	int pack;

	if (fb->conf.hf_access_flags & HPCFB_ACCESS_Y_TO_X)
		SWAP(x, y);
	if (fb->conf.hf_access_flags & HPCFB_ACCESS_R_TO_L)
		x = fb->conf.hf_width - x - 1;
	if (fb->conf.hf_access_flags & HPCFB_ACCESS_B_TO_T)
		y = fb->conf.hf_height - y - 1;

	if (x < 0 || y < 0 || fb->conf.hf_width <= x || fb->conf.hf_height < y)
		return;

	pack = x / fb->conf.hf_pixels_per_pack;
	x %= fb->conf.hf_pixels_per_pack;
	if (fb->conf.hf_access_flags & HPCFB_ACCESS_LSB_TO_MSB)
		x = fb->conf.hf_pixels_per_pack - x - 1;
	x *= fb->conf.hf_pixel_width;
	if (fb->conf.hf_access_flags & HPCFB_ACCESS_PACK_BLANK)
		x += (fb->conf.hf_pack_width - 
		    fb->conf.hf_pixel_width * fb->conf.hf_pixels_per_pack);
	x += pack * fb->conf.hf_pack_width;

	if (fb->linecache_y != y) {
		fb_flush(fb);
		fb_fetchline(fb, y);
	}

	__fb_put_pixel(fb, pixel, fb->conf.hf_pixel_width, x);
}

void
fb_drawline(struct fb *fb, int x0, int y0, int x1, int y1, fb_pixel_t pixel)
{
	int i, dx, dy, d, incdec;

	dx = ABS(x1 - x0);
	dy = ABS(y1 - y0);
	if (dx < dy) {
		if (y1 < y0) {
			SWAP(x0, x1);
			SWAP(y0, y1);
		}
		if (x0 < x1)
			incdec = 1;
		else
			incdec = -1;
		d = -dy;
		dx *= 2;
		dy *= 2;
		for (i = y0; i <= y1; i++) {
			fb_drawpixel(fb, x0, i, pixel);
			d += dx;
			if (0 <= d) {
				d -= dy;
				x0 += incdec;
			}
		}
	} else {
		if (x1 < x0) {
			SWAP(x0, x1);
			SWAP(y0, y1);
		}
		if (y0 < y1)
			incdec = 1;
		else
			incdec = -1;
		d = -dx;
		dx *= 2;
		dy *= 2;
		for (i = x0; i <= x1; i++) {
			fb_drawpixel(fb, i, y0, pixel);
			d += dy;
			if (0 <= d) {
				d -= dx;
				y0 += incdec;
			}
		}
	}
}
