/*
 * Copyright (c) 1993 Christian E. Hopps
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christian E. Hopps.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
#include "sys/param.h"
#include "grf_cc_priv.h"

/* local prototypes */
static void monitor_vbl_handler (monitor_t *m);
static dmode_t *get_next_mode (dmode_t *d);
static dmode_t *get_current_mode (void);
static dmode_t *get_best_mode (dimen_t *size, u_byte depth);
static void cc_set_flags (mdata_t *md);
    
/* bitmap functions */
static bmap_t *alloc_bitmap (u_word width, u_word height, u_word depth, u_word flags);
static void free_bitmap (bmap_t *bm);
void scroll_bitmap (bmap_t *bm, u_word x, u_word y, u_word width, u_word height, word dx, word dy, u_byte mask);

/* a pointer to us. */
static monitor_t *this;
static mdata_t *this_data;

/* the monitor name */
static char *monitor_name = "CCMONITOR";

/* the custom chip monitor structure. */
static monitor_t   monitor;
static mdata_t monitor_data;

/* our simple copper blanker copper list */
static cop_t *null_mode_copper_list;

extern dll_list_t *monitors;

/* monitor functions. */
monitor_t *
cc_init_monitor (void)
{
    if (!this) {
    	cop_t *cp;
	cc_monitor = this = &monitor;

	/* turn sprite DMA off. we don't support them yet. */
	custom.dmacon  =  DMAF_SPRITE;

	this->node.next = this->node.prev = NULL;
	this->name = monitor_name;
	this_data = this->data = &monitor_data;
	
	this->get_current_mode = get_current_mode;
	this->vbl_handler      = (vbl_handler_func *) monitor_vbl_handler;
	this->get_next_mode    = get_next_mode;
	this->get_best_mode    = get_best_mode;
	
	this->alloc_bitmap     = alloc_bitmap;
	this->free_bitmap      = free_bitmap;

	this_data->current_mode = NULL;
	dinit_list (&this_data->modes);

	cp = null_mode_copper_list = alloc_chipmem (sizeof (cop_t) * 4);
	if (!cp) {
	    panic ("no chipmem for grf.");
	}
	CMOVE (cp, R_COLOR00, 0x0000);		/* background is black */
	CMOVE (cp, R_BPLCON0, 0x0000);		/* no planes to fetch from */
	CWAIT (cp, 255, 255);			/* COPEND */
	CWAIT (cp, 255, 255);			/* COPEND really */

	/* install this list and turn DMA on */
	custom.cop1lc = PREP_DMA_MEM (null_mode_copper_list);
	custom.copjmp1 = 0;
	custom.dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER \
			| DMAF_COPPER;

	cc_init_modes();
	dadd_tail (monitors, &this->node);
    }
    return (this);
}

static void
monitor_vbl_handler (monitor_t *m)
{
    if (this_data->current_mode) {
	DMDATA(this_data->current_mode)->vbl_handler (this_data->current_mode);
    }
}

static dmode_t *
get_current_mode (void)
{
    if (this_data->current_mode) {
	return (this_data->current_mode);
    } else {
	return (NULL);
    }
}

static dmode_t *
get_next_mode (dmode_t *d)
{
    if (d) {
	if (d->node.next->next) {
	    return ((dmode_t *)d->node.next);
	}
    } else if (this_data->modes.head->next) {
	return ((dmode_t *)this_data->modes.head);
    } else {
	return (NULL);
    }
}

static dmode_t *
get_best_mode (dimen_t *size, u_byte depth)
{
    /* FIX: */
    dmode_t *save = NULL;
    dmode_t *m = (dmode_t *)this_data->modes.head;
    long dt;
    
    while (m->node.next) {
	long dx, dy, ct;
	dmdata_t *dmd = m->data;
	
	if (depth > dmd->max_depth || depth < dmd->min_depth) {
	    m = (dmode_t *)m->node.next;
	    continue;
	}
	if (size->width > dmd->max_size.width || size->height > dmd->max_size.height) {
	    m = (dmode_t *)m->node.next;
	    continue;
	}
	if (size->width < dmd->min_size.width || size->height < dmd->min_size.height) {
	    m = (dmode_t *)m->node.next;
	    continue;
	}
	
	dx = abs (m->nominal_size.width - size->width);
	dy = abs (m->nominal_size.height - size->height);
	ct = dx + dy;

	if (ct < dt || save == NULL) {
	    save = m;
	    dt = ct;
	}
	
	m = (dmode_t *)m->node.next;
    }
    return (save);
}

/* bitmap functions */
static bmap_t *
alloc_bitmap (u_word width, u_word height, u_word depth, u_word flags)
{
    int i;
    u_long total_size;
    u_word lwpr = (width+31)/32;
    u_word wpr = lwpr << 1;
    u_word bpr = wpr << 1;
    u_word array_size = sizeof (u_byte *) * depth;
    u_long plane_size = bpr*height;
    u_word temp_size = bpr + sizeof (u_long);
    bmap_t *bm;

    /* note the next allocation will give everything, also note that
     * all the stuff we want (including bitmaps) will be long word
     * aligned.  This is a function of the data being allocated and
     * the fact that alloc_chipmem() returns long word aligned data.
     * note also that each row of the bitmap is long word aligned and
     * made of exactly n longwords.
     * -ch
     */

    /* Sigh, it seems for mapping to work we need the bitplane data to
     * 1: be aligned on a page boundry.
     * 2: be n pages large.
     *
     * why? becuase the user gets a page aligned address, if this is
     * before your allocation, too bad.  Also it seems that the mapping
     * routines do not watch to closely to the allowable length. so if
     * you go over n pages by less than another page, the user gets to write
     * all over the entire page.  Since you did not allocate up to a page
     * boundry (or more) the user writes into someone elses memory.
     * -ch
     */
    total_size = amiga_round_page (plane_size*depth)+	/* for length */
                 (temp_size) + (array_size) + sizeof (bmap_t) + 
                 NBPG;	/* for alignment */
    bm = alloc_chipmem (total_size);
    if (bm) {
	if (flags & BMF_CLEAR) {
	    bzero (bm, total_size);
	}
	bm->bytes_per_row = bpr;
	bm->rows = height;
	bm->depth = depth;
	bm->flags = flags;
	bm->plane = (u_byte **)&bm[1];
	bm->blit_temp = ((u_byte *)bm->plane) + array_size;
	bm->plane[0] = (u_byte *)amiga_round_page ((u_long)(bm->blit_temp+temp_size));
	if (flags & BMF_INTERLEAVED) {
	    bm->row_mod = bm->bytes_per_row * (depth-1);
	    for (i = 1; i < depth; i++) {
	    	bm->plane[i] = bm->plane[i-1] + bpr; 
	    }
	} else {
	    bm->row_mod = 0;
	    for (i = 1; i < depth; i++) {
	    	bm->plane[i] = bm->plane[i-1] + plane_size;
	    }
	} 
	bm->hardware_address = PREP_DMA_MEM (bm->plane[0]);
	return (bm);
    }
    return (NULL);
}


static void
free_bitmap (bmap_t *bm)
{
    if (bm) {
	free_chipmem (bm);
    }    
}

/* load a new mode into the current display, if NULL shut display off. */
void
cc_load_mode (dmode_t *d)
{
    if (d) {
	this_data->current_mode = d;
	return;
    } 
    /* turn off display */
    this_data->current_mode = NULL;
    wait_tof ();
    wait_tof ();
    custom.cop1lc = PREP_DMA_MEM (null_mode_copper_list);
}
