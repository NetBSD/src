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

#include "grf_cc_priv.h"

static dmode_t *get_display_mode (view_t *v);

void
cc_init_view    (view_t *v, bmap_t *bm, dmode_t *mode, box_t *dbox)
{
    vdata_t *vd = VDATA(v);
    v->bitmap = bm;
    vd->mode = mode;
    bcopy (dbox, &v->display, sizeof (box_t));

    v->display_view = DMDATA(vd->mode)->display_view;
    v->use_colormap = DMDATA(vd->mode)->use_colormap;
    v->get_colormap = DMDATA(vd->mode)->get_colormap;
    v->free_view =    cc_free_view;
    v->get_display_mode = get_display_mode;
    v->remove_view =  cc_remove_view;
}

void
cc_free_view (view_t *v)
{
    if (v) {
	vdata_t *vd = VDATA(v);
	dmode_t *md = vd->mode;
	v->remove_view (v);
	cc_monitor->free_bitmap (v->bitmap);
	free_chipmem (v);
    }
}

void
cc_remove_view  (view_t *v)
{
    dmode_t *mode = VDATA (v)->mode;

    if (MDATA(cc_monitor)->current_mode == mode) {
    	if (DMDATA(mode)->current_view == v) {
	    cc_load_mode (NULL);
    	}	        
    } 
    if (DMDATA(mode)->current_view == v) {
	DMDATA(mode)->current_view = NULL;
    }	        
    
    VDATA (v)->flags &= ~VF_DISPLAY;
}

static dmode_t *
get_display_mode (view_t *v)
{
    return (VDATA(v)->mode);
}

