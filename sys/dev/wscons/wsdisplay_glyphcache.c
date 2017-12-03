/*	$NetBSD: wsdisplay_glyphcache.c,v 1.3.2.3 2017/12/03 11:37:37 jdolecek Exp $	*/

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
 */

#ifdef _KERNEL_OPT
#include "opt_glyphcache.h"
#endif
 
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>

#ifdef GLYPHCACHE_DEBUG
#define DPRINTF aprint_normal
#else
#define DPRINTF while (0) printf
#endif

#define NBUCKETS 32

static inline int
attr2idx(long attr)
{
	if ((attr & 0xf0f0fff8) != 0)
		return -1;
	
	return (((attr >> 16) & 0x0f) | ((attr >> 20) & 0xf0));
}

/* first line, lines, width, attr */
int
glyphcache_init(glyphcache *gc, int first, int lines, int width,
    int cellwidth, int cellheight, long attr)
{

	/* first the geometry stuff */
	if (lines < 0) lines = 0;
	gc->gc_width = width;
	gc->gc_cellwidth = -1;
	gc->gc_cellheight = -1;
	gc->gc_firstline = first;
	gc->gc_lines = lines;
	gc->gc_buckets = NULL;
	gc->gc_numbuckets = 0;
	// XXX: Never free?
	gc->gc_buckets = kmem_alloc(sizeof(*gc->gc_buckets) * NBUCKETS,
	    KM_SLEEP);
	gc->gc_nbuckets = NBUCKETS;
	return glyphcache_reconfig(gc, cellwidth, cellheight, attr);

}

int
glyphcache_reconfig(glyphcache *gc, int cellwidth, int cellheight, long attr)
{
	int cache_lines, buckets, i, usedcells = 0, idx;
	gc_bucket *b;

	/* see if we actually need to reconfigure anything */
	if ((gc->gc_cellwidth == cellwidth) &&
	    (gc->gc_cellheight == cellheight) &&
	    ((gc->gc_buckets != NULL) &&
	     (gc->gc_buckets[0].gb_index == attr2idx(attr)))) {
		return 0;
	}

	gc->gc_cellwidth = cellwidth;
	gc->gc_cellheight = cellheight;

	gc->gc_cellsperline = gc->gc_width / cellwidth;

	cache_lines = gc->gc_lines / cellheight;
	gc->gc_numcells = cache_lines * gc->gc_cellsperline;

	/* now allocate buckets */
	buckets = (gc->gc_numcells / 223);
	if ((buckets * 223) < gc->gc_numcells)
		buckets++;

	/*
	 * if we don't have enough video memory to cache at least a few glyphs
	 * we stop right here
	 */
	if (buckets < 1)
		return ENOMEM;

	buckets = min(buckets, gc->gc_nbuckets);
	gc->gc_numbuckets = buckets;

	DPRINTF("%s: using %d buckets\n", __func__, buckets);
	for (i = 0; i < buckets; i++) {
		b = &gc->gc_buckets[i];
		b->gb_firstcell = usedcells;
		b->gb_numcells = min(223, gc->gc_numcells - usedcells);
		usedcells += 223;
		b->gb_usedcells = 0;
		b->gb_index = -1;
	}

	/* initialize the attribute map... */
	for (i = 0; i < 256; i++) {
		gc->gc_attrmap[i] = -1;
	}

	/* first bucket goes to default attr */
	idx = attr2idx(attr);
	if (idx >= 0) {
		gc->gc_attrmap[idx] = 0;
		gc->gc_buckets[0].gb_index = idx;
	}

	glyphcache_wipe(gc);
	DPRINTF("%s: using %d cells total, from %d width %d\n", __func__,
	    gc->gc_numcells, gc->gc_firstline, gc->gc_cellsperline);
	return 0;
}

void
glyphcache_adapt(struct vcons_screen *scr, void *cookie)
{
	glyphcache *gc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	if (ri->ri_wsfcookie != gc->gc_fontcookie) {
		glyphcache_wipe(gc);
		gc->gc_fontcookie = ri->ri_wsfcookie;
	}

	glyphcache_reconfig(gc, ri->ri_font->fontwidth,
			        ri->ri_font->fontheight, scr->scr_defattr);
} 

void
glyphcache_wipe(glyphcache *gc)
{
	gc_bucket *b;
	int i, j, idx;

	if ((gc->gc_buckets == NULL) || (gc->gc_numbuckets < 1))
		return;

	idx = gc->gc_buckets[0].gb_index;

	/* empty all the buckets */
	for (i = 0; i < gc->gc_numbuckets; i++) {
		b = &gc->gc_buckets[i];
		b->gb_usedcells = 0;
		b->gb_index = -1;
		for (j = 0; j < b->gb_numcells; j++)
			b->gb_map[j] = -1;
	}

	for (i = 0; i < 256; i++) {
		gc->gc_attrmap[i] = -1;
	}

	/* now put the first bucket back where it was */
	gc->gc_attrmap[idx] = 0;
	gc->gc_buckets[0].gb_index = idx;
}

/*
 * add a glyph drawn at (x,y) to the cache as (c)
 * call this only if glyphcache_try() returned GC_ADD
 * caller or gc_bitblt must make sure the glyph is actually completely drawn
 */
int
glyphcache_add(glyphcache *gc, int c, int x, int y)
{
	gc_bucket *b = gc->gc_next;
	int cell;
	int cx, cy;

	if (b->gb_usedcells >= b->gb_numcells)
		return ENOMEM;
	cell = atomic_add_int_nv(&b->gb_usedcells, 1) - 1;
	cell += b->gb_firstcell;
	cy = gc->gc_firstline +
	    (cell / gc->gc_cellsperline) * gc->gc_cellheight;
	cx = (cell % gc->gc_cellsperline) * gc->gc_cellwidth;
	b->gb_map[c - 33] = (cx << 16) | cy;
	gc->gc_bitblt(gc->gc_blitcookie, x, y, cx, cy,
	    gc->gc_cellwidth, gc->gc_cellheight, gc->gc_rop);
	if (gc->gc_underline & 1) {
		glyphcache_underline(gc, x, y, gc->gc_underline);
	}
	return 0;
}

void
glyphcache_underline(glyphcache *gc, int x, int y, long attr)
{
	if (gc->gc_rectfill == NULL)
		return;

	gc->gc_rectfill(gc->gc_blitcookie, x, y + gc->gc_cellheight - 2,
	    gc->gc_cellwidth, 1, attr);
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
	int cell, cx, cy, idx, bi;
	gc_bucket *b;

	idx = attr2idx(attr);
	/* see if we're in range */
	if ((c < 33) || (c > 255) || (idx < 0))
		return GC_NOPE;
	/* see if there's already a bucket for this attribute */
	bi = gc->gc_attrmap[idx];
	if (bi == -1) {
		/* nope, see if there's an empty one left */
		bi = 1;
		while ((bi < gc->gc_numbuckets) && 
		       (gc->gc_buckets[bi].gb_index != -1)) {
			bi++;
		}
		if (bi < gc->gc_numbuckets) {
			/* found one -> grab it */
			gc->gc_attrmap[idx] = bi;
			b = &gc->gc_buckets[bi];
			b->gb_index = idx;
			b->gb_usedcells = 0;
			/* make sure this doesn't get evicted right away */
			b->gb_lastread = time_uptime;
		} else {
			/*
			 * still nothing
			 * steal the least recently read bucket
			 */
			time_t moo = time_uptime;
			int i, oldest = 1;

			for (i = 1; i < gc->gc_numbuckets; i++) {
				if (gc->gc_buckets[i].gb_lastread < moo) {
					oldest = i;
					moo = gc->gc_buckets[i].gb_lastread;
				}
			}

			/* if we end up here all buckets must be in use */
			b = &gc->gc_buckets[oldest];
			gc->gc_attrmap[b->gb_index] = -1;
			b->gb_index = idx;
			b->gb_usedcells = 0;
			gc->gc_attrmap[idx] = oldest;
			/* now scrub it */
			for (i = 0; i < b->gb_numcells; i++)
				b->gb_map[i] = -1;
			/* and set the time stamp */
			b->gb_lastread = time_uptime;
		}
	} else {
		/* found one */
		b = &gc->gc_buckets[bi];
	}

	/* see if there's room in the bucket */
	if (b->gb_usedcells >= b->gb_numcells)
		return GC_NOPE;

	cell = b->gb_map[c - 33];
	if (cell == -1) {
		gc->gc_next = b;
		gc->gc_underline = attr;
		return GC_ADD;
	}

	/* it's in the cache - draw it */
	cy = cell & 0xffff;
	cx = (cell >> 16) & 0xffff;
	gc->gc_bitblt(gc->gc_blitcookie, cx, cy, x, y,
	    gc->gc_cellwidth, gc->gc_cellheight, gc->gc_rop);
	/* and underline it if needed */
	if (attr & 1)
		glyphcache_underline(gc, x, y, attr);
	/* update bucket's time stamp */
	b->gb_lastread = time_uptime;
	return GC_OK;
}
