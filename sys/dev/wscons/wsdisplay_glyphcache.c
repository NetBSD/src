/*	$NetBSD: wsdisplay_glyphcache.c,v 1.1.4.2 2012/04/17 00:08:11 yamt Exp $	*/

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

/* 
 * a simple glyph cache in offscreen memory
 * For now it only caches glyphs with the default attribute ( assuming they're
 * the most commonly used glyphs ) but the API should at least not prevent
 * more sophisticated caching algorithms
 */

#include <sys/atomic.h>
#include <sys/errno.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>

/* first line, lines, width, attr */
int
glyphcache_init(glyphcache *gc, int first, int lines, int width,
    int cellwidth, int cellheight, long attr)
{
	int cache_lines;

	gc->gc_cellwidth = cellwidth;
	gc->gc_cellheight = cellheight;
	gc->gc_firstline = first;
	gc->gc_cellsperline = width / cellwidth;
	cache_lines = lines / cellheight;
	gc->gc_numcells = cache_lines * gc->gc_cellsperline;
	if (gc->gc_numcells > 256)
		gc->gc_numcells = 256;
	gc->gc_attr = attr;
	glyphcache_wipe(gc);
	return 0;
}

void
glyphcache_wipe(glyphcache *gc)
{
	int i;

	gc->gc_usedcells = 0;
	for (i = 0; i < 256; i++)
		gc->gc_map[i] = -1;
}

/*
 * add a glyph drawn at (x,y) to the cache as (c)
 * call this only if glyphcache_try() returned GC_ADD
 * caller or gc_bitblt must make sure the glyph is actually completely drawn
 */
int
glyphcache_add(glyphcache *gc, int c, int x, int y)
{
	int cell;
	int cx, cy;

	if (gc->gc_map[c] != -1)
		return EINVAL;
	if (gc->gc_usedcells >= gc->gc_numcells)
		return ENOMEM;
	cell = atomic_add_int_nv(&gc->gc_usedcells, 1) - 1;
	gc->gc_map[c] = cell;
	cy = gc->gc_firstline +
	    (cell / gc->gc_cellsperline) * gc->gc_cellheight;
	cx = (cell % gc->gc_cellsperline) * gc->gc_cellwidth;
	gc->gc_bitblt(gc->gc_blitcookie, x, y, cx, cy,
	    gc->gc_cellwidth, gc->gc_cellheight, gc->gc_rop);
	return 0;
}

/*
 * check if (c) is in the cache, if so draw it at (x,y)
 * return:
 * - GC_OK when the glyph was found
 * - GC_ADD when the glyph wasn't found but can be added
 * - GC_NOPE when the glyph can't be cached
 */
int
glyphcache_try(glyphcache *gc, int c, int x, int y, long attr)
{
	int cell, cx, cy;
	if ((c < 0) || (c > 255) || (attr != gc->gc_attr))
		return GC_NOPE;
	if (gc->gc_usedcells >= gc->gc_numcells)
		return GC_NOPE;
	cell = gc->gc_map[c];
	if (cell == -1)
		return GC_ADD;
	cy = gc->gc_firstline +
	    (cell / gc->gc_cellsperline) * gc->gc_cellheight;
	cx = (cell % gc->gc_cellsperline) * gc->gc_cellwidth;
	gc->gc_bitblt(gc->gc_blitcookie, cx, cy, x, y,
	    gc->gc_cellwidth, gc->gc_cellheight, gc->gc_rop);
	return GC_OK;
}
