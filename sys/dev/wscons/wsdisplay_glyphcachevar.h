/*	$NetBSD: wsdisplay_glyphcachevar.h,v 1.1.4.2 2012/04/17 00:08:11 yamt Exp $	*/

/*
 * Copyright (c) 2012 Michael Lorenz
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

/* a simple glyph cache in offscreen memory */

#ifndef WSDISPLAY_GLYPHCACHEVAR_H
#define WSDISPLAY_GLYPHCACHEVAR_H

typedef struct _glyphcache {
	/* mapping char codes to cache cells */
	volatile unsigned int gc_usedcells;
	int gc_numcells;
	int gc_map[256];
	/* geometry */
	int gc_cellwidth;
	int gc_cellheight;
	int gc_cellsperline;
	int gc_firstline;	/* first line in vram to use for glyphs */
	long gc_attr;
	/*
	 * method to copy glyphs within vram,
	 * to be initialized before calling glyphcache_init()
	 */
	void (*gc_bitblt)(void *, int, int, int, int, int, int, int);
	void *gc_blitcookie;
	int gc_rop;
} glyphcache;

/* first line, lines, width, cellwidth, cellheight, attr */
int glyphcache_init(glyphcache *, int, int, int, int, int, long);
/* clear out the cache, for example when returning from X */
void glyphcache_wipe(glyphcache *);
/* add a glyph to the cache */
int glyphcache_add(glyphcache *, int, int, int); /* char code, x, y */
/* try to draw a glyph from the cache */
int glyphcache_try(glyphcache *, int, int, int, long); /* char code, x, y, attr */
#define	GC_OK	0 /* glyph was in cache and has been drawn */
#define GC_ADD	1 /* glyph is not in cache but can be added */
#define GC_NOPE 2 /* glyph is not in cache and can't be added */

#endif /* WSDISPLAY_GLYPHCACHEVAR_H */