/*
 * Copyright (c) 1994 Christian E. Hopps
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 *	$Id: grf_cc_mode.c,v 1.3 1994/01/30 08:25:09 chopps Exp $
 */
#include "errno.h"
#include "grf_cc_priv.h"

#if defined (GRF_A2024)
#include "../../../amiga/cc_2024.h"
#endif /* GRF_A2024 */

#include "sys/cdefs.h"
#include "machine/param.h"

dmode_t *(*mode_init_funcs[])(void) = {
#if defined (GRF_NTSC)
#if defined (GRF_A2024)
    cc_init_ntsc_a2024,
    cc_init_ntsc_hires_dlace,
#endif /* GRF_A2024 */
    cc_init_ntsc_hires_lace,
    cc_init_ntsc_hires,
#endif /* GRF_NTSC */
#if defined (GRF_PAL)
#if defined (GRF_A2024)
    cc_init_pal_a2024,
    cc_init_pal_hires_dlace,
#endif /* GRF_A2024 */
    cc_init_pal_hires_lace,
    cc_init_pal_hires,
#endif /* GRF_PAL */
    NULL
};

int
cc_init_modes (void)
{
    int i = 0;
    int error = 0;
    while (mode_init_funcs[i]) {
	mode_init_funcs[i] ();
	i++;
    }
    return (error);
}

/*
 * Mode functions.
 */


monitor_t *
cc_get_monitor (dmode_t *d)
{
    return (DMDATA(d)->monitor);
}

view_t *
cc_get_current_view (dmode_t *d)
{
    return (DMDATA(d)->current_view);
}

void
cc_mode_vbl_handler (dmode_t *m)
{
}

view_t *
cc_alloc_view   (dmode_t *mode, dimen_t *dim, u_byte depth)
{
    view_t *v = alloc_chipmem (sizeof (*v) + sizeof (vdata_t));
    if (v) {
	bmap_t *bm = cc_monitor->alloc_bitmap (dim->width, dim->height, depth, BMF_CLEAR);
	if (bm) {
	    int i;
	    box_t box;

	    v->data = &v[1];			  /* at the end of view */
	    VDATA(v)->colormap = DMDATA(mode)->alloc_colormap (depth);
	    if (VDATA(v)->colormap) {
	    	INIT_BOX (&box, 0, 0, dim->width, dim->height);
	    	cc_init_view (v, bm, mode, &box);
	    	return (v);
	    }
	    cc_monitor->free_bitmap (bm);
	}
	free_chipmem (v);
    }
    return (NULL);
}

u_word cc_default_colors[32] = {
    0xAAA, 0x000, 0x68B, 0xFFF,
    0x369, 0x963, 0x639, 0x936,
    0x000, 0x00F, 0x0F0, 0xF00, 
    0x0FF, 0xFF0, 0xF0F, 0xFFF,
    0x000, 0x111, 0x222, 0x333,
    0x444, 0x555, 0x666, 0x777,
    0x888, 0x999, 0xAAA, 0xBBB,
    0xCCC, 0xDDD, 0xEEE, 0xFFF
};

#if defined (GRF_A2024)
u_word cc_a2024_default_colors[4] = {
    0x0, /* BLACK */
    0x3, /* WHITE */
    0x2, /* LGREY */
    0x1  /* DGREY */
};
#endif /* GRF_A2024 */

colormap_t *
cc_alloc_colormap (int depth)
{
    u_long size = 1u << depth, i;
    colormap_t *cm = alloc_chipmem (sizeof (u_long)*size + sizeof (*cm));

    if (cm) {
    	cm->type = CM_COLOR;
	cm->red_mask = 0x0F;
	cm->green_mask = 0x0F;
	cm->blue_mask = 0x0F;
	cm->first = 0;
	cm->size = size;
	cm->entry = (u_long *) &cm[1]; /* table directly after. */
	for (i=0; i < min(size,32); i++) {
	    cm->entry[i] = CM_WTOL(cc_default_colors[i]);
	}
	return (cm);
    }
    return (NULL);
}

#if defined (GRF_A2024)
colormap_t *
cc_a2024_alloc_colormap (int depth)
{
    u_long size = 1u << depth, i;
    colormap_t *cm = alloc_chipmem (sizeof (u_long)*size + sizeof (*cm));

    if (cm) {
    	cm->type = CM_GREYSCALE;
	cm->grey_mask = 0x03;
	cm->first = 0;
	cm->size = size;
	cm->entry = (u_long *) &cm[1]; /* table directly after. */
	for (i=0; i < size; i++) {
	    cm->entry[i] = CM_WTOL(cc_a2024_default_colors[i]);
	}
	return (cm);
    }
    return (NULL);
}
#endif /* GRF_A2024 */

int 
cc_colormap_checkvals (colormap_t *vcm, colormap_t *cm, int use)
{
    if (use) {
	/* check to see if its the view's colormap, if so just do update. */
	if (vcm != cm) {
	    if (cm->first >= vcm->size || (cm->first + cm->size) > (cm->first + vcm->size) ||
	    	cm->type != vcm->type) {
		return (0);
	    }

	    switch (vcm->type) {
	      case CM_COLOR:
		if (cm->red_mask != vcm->red_mask ||
		    cm->green_mask != vcm->green_mask ||
		    cm->blue_mask != vcm->blue_mask) {
		    return (0);
		}
		break;
	      case CM_GREYSCALE:
		if (cm->grey_mask != vcm->grey_mask) {
		    return (0);
		}
		break;
	    }
	} 
    } else {
    	if (cm->first >= vcm->size || (cm->first + cm->size) > (cm->first + vcm->size)) {
	    return (0);
	}
    }
    return (1);
}

/* does sanity check on values */
int 
cc_get_colormap (view_t *v, colormap_t *cm)
{
    colormap_t *vcm = VDATA(v)->colormap;
    int i;

    if (!cc_colormap_checkvals (vcm, cm, 0)) {
        return (EINVAL);
    }

    cm->type = vcm->type;

    switch (vcm->type) {
      case CM_COLOR:
      	cm->red_mask = vcm->red_mask;
      	cm->green_mask = vcm->green_mask;
      	cm->blue_mask = vcm->blue_mask;
      	break;
      case CM_GREYSCALE:
        cm->grey_mask = vcm->grey_mask;
        break;
    }

    /* copy entries into colormap. */
    for (i = cm->first; i < (cm->first+cm->size); i++) {
	cm->entry[i] = vcm->entry[i]; 
    }
    return (0);
}

#if defined (GRF_A2024)
int 
cc_a2024_get_colormap (view_t *v, colormap_t *cm)
{
    /* there are no differences (yet) in the way the cm's are stored */
    return (cc_get_colormap (v,cm));
}
#endif /* GRF_A2024 */

/* does sanity check on values */
int 
cc_use_colormap (view_t *v, colormap_t *cm)
{
    colormap_t *vcm = VDATA(v)->colormap;
    int s, i;

    if (!cc_colormap_checkvals (vcm, cm, 1)) {
        return (EINVAL);
    }

    /* check to see if its the view's colormap, if so just do update. */
    if (vcm != cm) {
	/* copy entries into colormap. */
    	for (i = cm->first; i < (cm->first+cm->size); i++) {
	    vcm->entry[i] = cm->entry[i]; 
    	}	
    }
    s = spltty ();

    /* is view currently being displayed? */
    if (VDATA(v)->flags & VF_DISPLAY) {
	/* yes, update the copper lists */
    	cop_t *tmp, *cp;
    	int nframes = 1, j; 

	if (DMDATA(VDATA(v)->mode)->flags & DMF_INTERLACE) {
	    nframes = 2;
	}

	for (i = 0; i < nframes; i++) {
	    cp = DMDATA(VDATA(v)->mode)->frames[i]; 

	    tmp = find_copper_inst (cp, CI_MOVE(R_COLOR07));
	    tmp -= 7;

	    for (j=0; j<16; j++) {
	        CMOVE (tmp, R_COLOR00+(j<<1), CM_LTOW (vcm->entry[j]));
	    }
	}
    }

    splx (s);
    return (0);
}

#if defined (GRF_A2024)
int 
cc_a2024_use_colormap (view_t *v, colormap_t *cm)
{
    colormap_t *vcm = VDATA(v)->colormap;
    int s, i;

    if (!cc_colormap_checkvals (vcm, cm, 1)) {
        return (EINVAL);
    }

    /* check to see if its the view's colormap, if so just do update. */
    if (vcm != cm) {
	/* copy entries into colormap. */
    	for (i = cm->first; i < (cm->first+cm->size); i++) {
	    vcm->entry[i] = cm->entry[i]; 
    	}	
    }
    s = spltty ();

    /* is view currently being displayed? */
    if (VDATA(v)->flags & VF_DISPLAY) { 
    	/* yes, update the copper lists */ 
    	cop_t *tmp, *cp;
    	int nframes = 2, nregs = cm->size == 4 ? 16 : 8, j; 

	if (DMDATA(VDATA(v)->mode)->flags & DMF_HEDLEY_EXP) {
	    nframes = 4;
	}

	for (i = 0; i < nframes; i++) {
	    cp = DMDATA(VDATA(v)->mode)->frames[i]; 

	    tmp = find_copper_inst (cp, CI_MOVE(R_COLOR07));
	    tmp -= 7;

	    for (j=0; j<nregs; j++) {
	        CMOVE (tmp, R_COLOR00+(j<<1), A2024_CM_TO_CR (vcm, j));
	    }
	}
    }

    splx (s);
    return (0);
}
#endif /* GRF_A2024 */

