/* 	$NetBSD: wsfont.c,v 1.12 2000/01/07 03:25:46 enami Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andy Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wsfont.c,v 1.12 2000/01/07 03:25:46 enami Exp $");

#include "opt_wsfont.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/malloc.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>

#undef HAVE_FONT

#ifdef FONT_QVSS8x15
#define HAVE_FONT 1
#include <dev/wsfont/qvss8x15.h>
#endif

#ifdef FONT_GALLANT12x22
#define HAVE_FONT 1
#include <dev/wsfont/gallant12x22.h>
#endif

#ifdef FONT_LUCIDA16x29
#define HAVE_FONT 1
#include <dev/wsfont/lucida16x29.h>
#endif

#ifdef FONT_VT220L8x8
#define HAVE_FONT 1
#include <dev/wsfont/vt220l8x8.h>
#endif

#ifdef FONT_VT220L8x10
#define HAVE_FONT 1
#include <dev/wsfont/vt220l8x10.h>
#endif

/* Make sure we always have at least one font. */
#ifndef HAVE_FONT
#define HAVE_FONT 1
#define FONT_BOLD8x16 1
#endif

#ifdef FONT_BOLD8x16
#include <dev/wsfont/bold8x16.h>
#endif

/* Placeholder struct used for linked list */
struct font {
	struct	font *next;
	struct	font *prev;
	struct	wsdisplay_font *font;
	u_short	lockcount;
	u_short	cookie;
	u_short	flg;
};	

/* Our list of built-in fonts */
static struct font *list, builtin_fonts[] = {
#ifdef FONT_BOLD8x16
	{ NULL, NULL, &bold8x16, 0, 1, WSFONT_STATIC | WSFONT_BUILTIN  },
#endif
#ifdef FONT_ISO8x16
	{ NULL, NULL, &iso8x16, 0, 2, WSFONT_STATIC | WSFONT_BUILTIN },
#endif
#ifdef FONT_COURIER11x18
	{ NULL, NULL, &courier11x18, 0, 3, WSFONT_STATIC | WSFONT_BUILTIN },
#endif
#ifdef FONT_GALLANT12x22
	{ NULL, NULL, &gallant12x22, 0, 4, WSFONT_STATIC | WSFONT_BUILTIN },
#endif
#ifdef FONT_LUCIDA16x29
	{ NULL, NULL, &lucida16x29, 0, 5, WSFONT_STATIC | WSFONT_BUILTIN },
#endif
#ifdef FONT_QVSS8x15
	{ NULL, NULL, &qvss8x15, 0, 6, WSFONT_STATIC | WSFONT_BUILTIN },
#endif
#ifdef FONT_VT220L8x8
	{ NULL, NULL, &vt220l8x8, 0, 7, WSFONT_STATIC | WSFONT_BUILTIN },
#endif
#ifdef FONT_VT220L8x10
	{ NULL, NULL, &vt220l8x10, 0, 8, WSFONT_STATIC | WSFONT_BUILTIN },
#endif
	{ NULL, NULL, NULL, 0 },
};

/* Reverse the bit order in a byte */
static const u_char reverse[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff, 
};

static struct	font *wsfont_find0 __P((int));
static void	wsfont_revbit __P((struct wsdisplay_font *));
static void	wsfont_revbyte __P((struct wsdisplay_font *));

/*
 * Reverse the bit order of a font
 */
static void
wsfont_revbit(font)
	struct wsdisplay_font *font;
{
	u_char *p, *m;
	
	p = (u_char *)font->data;
	m = p + font->stride * font->numchars * font->fontheight;
	
	while (p < m)
		*p++ = reverse[*p];
}

/*
 * Reverse the byte order of a font
 */
static void
wsfont_revbyte(font)
	struct wsdisplay_font *font;
{
	int x, l, r, nr;
	u_char *rp;
	
	if (font->stride == 1)
		return;

	rp = (u_char *)font->data;
	nr = font->numchars * font->fontheight;
	
	while (nr--) {
		l = 0;
		r = font->stride - 1;
		
		while (l < r) {
			x = rp[l];
			rp[l] = rp[r];
			rp[r] = x;
			l++, r--;
		}
		
		rp += font->stride;
	}
}

/*
 * Enumarate the list of fonts
 */
void
wsfont_enum(cb)
	void (*cb) __P((char *, int, int, int));
{
	struct wsdisplay_font *f;
	struct font *ent;
	int s;
	
	s = splhigh();
	
	for (ent = list; ent; ent = ent->next) {
		f = ent->font;	
		cb(f->name, f->fontwidth, f->fontheight, f->stride);
	}
	
	splx(s);
}

/*
 * Initialize list with WSFONT_BUILTIN fonts
 */
void
wsfont_init(void)
{
	static int again;
	int i;
	
	if (again != 0)
		return;
	again = 1;
		
	for (i = 0; builtin_fonts[i].font != NULL; i++) {
		builtin_fonts[i].next = list;
		list = &builtin_fonts[i];
	}
}

/*
 * Find a font by cookie. Called at splhigh.
 */
static struct font *
wsfont_find0(cookie)
	int cookie;
{
	struct font *ent;
	
	for (ent = list; ent != NULL; ent = ent->next)
		if (ent->cookie == cookie)
			return (ent);
			
	return (NULL);
}

/*
 * Find a font.
 */
int
wsfont_find(name, width, height, stride)
	char *name;
	int width, height, stride;
{
	struct font *ent;
	int s;
	
	s = splhigh();
	
	for (ent = list; ent != NULL; ent = ent->next) {
		if (height != 0 && ent->font->fontheight != height)
			continue;

		if (width != 0 && ent->font->fontwidth != width)
			continue;

		if (stride != 0 && ent->font->stride != stride)
			continue;
		
		if (name != NULL && strcmp(ent->font->name, name) != 0)
			continue;

		splx(s);
		return (ent->cookie);
	}

	splx(s);
	return (-1);
}

/*
 * Add a font to the list.
 */
#ifdef notyet
int
wsfont_add(font, copy)
	struct wsdisplay_font *font;
	int copy;
{
	static int cookiegen = 666;
	struct font *ent;
	size_t size;
	int s;
	
	s = splhigh();
	
	/* Don't allow exact duplicates */
	if (wsfont_find(font->name, font->fontwidth, font->fontheight, 
	    font->stride) >= 0) {
		splx(s);
		return (-1);
	}
	
	MALLOC(ent, struct font *, sizeof *ent, M_DEVBUF, M_WAITOK);
	
	ent->lockcount = 0;
	ent->flg = 0;
	ent->cookie = cookiegen++;
	ent->next = list;
	ent->prev = NULL;
	
	/* Is this font statically allocated? */
	if (!copy) {
		ent->font = font;
		ent->flg = WSFONT_STATIC;
	} else {
		MALLOC(ent->font, struct wsdisplay_font *, sizeof *ent->font, 
		    M_DEVBUF, M_WAITOK);
		memcpy(ent->font, font, sizeof(*ent->font));
		
		size = font->fontheight * font->numchars * font->stride;
		MALLOC(ent->font->data, void *, size, M_DEVBUF, M_WAITOK);
		memcpy(ent->font->data, font->data, size);
		ent->flg = 0;
	}
	
	/* Now link into the list and return */
	list = ent;
	splx(s);	
	return (0);
}
#endif
			
/*
 * Remove a font.
 */
#ifdef notyet
int
wsfont_remove(cookie)
	int cookie;
{
	struct font *ent;
	int s;
	
	s = splhigh();

	if ((ent = wsfont_find0(cookie)) == NULL) {
		splx(s);
		return (-1);
	}
	
	if ((ent->flg & WSFONT_BUILTIN) != 0 || ent->lockcount != 0) {
		splx(s);
		return (-1);
	}
	
	/* Don't free statically allocated font data */
	if ((ent->flg & WSFONT_STATIC) != 0) {
		FREE(ent->font->data, M_DEVBUF);
		FREE(ent->font, M_DEVBUF);
	}
		
	/* Remove from list, free entry */	
	if (ent->prev)
		ent->prev->next = ent->next;
	else
		list = ent->next;
			
	if (ent->next)
		ent->next->prev = ent->prev;	
			
	FREE(ent, M_DEVBUF);
	splx(s);
	return (0);
}
#endif

/*
 * Lock a given font and return new lockcount. This fails if the cookie
 * is invalid, or if the font is already locked and the bit/byte order 
 * requested by the caller differs.
 */
int
wsfont_lock(cookie, ptr, bitorder, byteorder)
	int cookie;
	struct wsdisplay_font **ptr;
	int bitorder, byteorder;
{
	struct font *ent;
	int s, lc;
	
	s = splhigh();
	
	if ((ent = wsfont_find0(cookie)) != NULL) {
		if (bitorder && bitorder != ent->font->bitorder) {
			if (ent->lockcount) {
				splx(s);
				return (-1);
			}
			wsfont_revbit(ent->font);
			ent->font->bitorder = bitorder;
		}

		if (byteorder && byteorder != ent->font->byteorder) {
			if (ent->lockcount) {
				splx(s);
				return (-1);
			}
			wsfont_revbyte(ent->font);
			ent->font->byteorder = byteorder;
		}
		
		lc = ++ent->lockcount;
		*ptr = ent->font;
	} else
		lc = -1;
	
	splx(s);
	return (lc);
}

/*
 * Get font flags and lockcount.
 */
int
wsfont_getflg(cookie, flg, lc)
	int cookie, *flg, *lc;
{
	struct font *ent;
	int s;
	
	s = splhigh();
	
	if ((ent = wsfont_find0(cookie)) != NULL) {
		*flg = ent->flg;
		*lc = ent->lockcount;
	}
	
	splx(s);
	return (ent != NULL ? 0 : -1);
}

/*
 * Unlock a given font and return new lockcount.
 */
int
wsfont_unlock(cookie)
	int cookie;
{
	struct font *ent;
	int s, lc;
	
	s = splhigh();
	
	if ((ent = wsfont_find0(cookie)) != NULL) {
		if (ent->lockcount == 0)
			panic("wsfont_unlock: font not locked\n");
		lc = --ent->lockcount;
	} else	
		lc = -1;
	
	splx(s);
	return (lc);
}
