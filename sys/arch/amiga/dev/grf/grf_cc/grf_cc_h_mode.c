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

#if defined (GRF_NTSC)

#include "grf_cc_priv.h"

static void hires_mode_vbl_handler (dmode_t *d);
static void display_hires_view (view_t *v);
static view_t *get_current_view (dmode_t *d);

enum frame_numbers {
    F_LONG,
    F_STORE_LONG,
    F_TOTAL
};

static dmode_t hires_mode;
static dmdata_t hires_mode_data;
static cop_t *hires_frames[F_TOTAL];
static dmode_t *this;
static dmdata_t *this_data;

dmode_t *
cc_init_ntsc_hires (void)
{
    /* this function should only be called once. */
    if (!this) {
	u_word len = std_copper_list_len;
	cop_t *cp;
	
	this = &hires_mode;
	this_data = &hires_mode_data;
	bzero (this, sizeof (dmode_t));
	bzero (this_data, sizeof (dmdata_t));
	
	this->name = "ntsc: hires interlace";
	this->nominal_size.width = 640;
	this->nominal_size.height = 200;
	this_data->max_size.width = 724;
	this_data->max_size.height = 241;
	this_data->min_size.width = 320;
	this_data->min_size.height = 100;
	this_data->min_depth = 1;
	this_data->max_depth = 4;
	this->data = this_data;

	this->get_monitor = cc_get_monitor;
	this->alloc_view = cc_alloc_view;
	this->get_current_view = get_current_view;

	this_data->use_colormap = cc_use_colormap;
	this_data->get_colormap = cc_get_colormap;
	this_data->alloc_colormap = cc_alloc_colormap;
	this_data->display_view = display_hires_view;
	this_data->monitor = cc_monitor;

	this_data->frames = hires_frames;
	this_data->frames[F_LONG] = alloc_chipmem (std_copper_list_size*F_TOTAL);
	if (!this_data->frames[F_LONG]) {
	    panic ("couldn't get chipmem for copper list");
	}
	
	this_data->frames[F_STORE_LONG] = &this_data->frames[F_LONG][len];
	
	bcopy (std_copper_list, this_data->frames[F_STORE_LONG], std_copper_list_size);
	bcopy (std_copper_list, this_data->frames[F_LONG], std_copper_list_size);
	
	this_data->bplcon0 = 0x8200;		  /* hires, color composite enable, lace. */
	this_data->std_start_x = STANDARD_VIEW_X;
	this_data->std_start_y = STANDARD_VIEW_Y;
	this_data->vbl_handler = (vbl_handler_func *) hires_mode_vbl_handler;
#if defined (GRF_ECS)
	this_data->beamcon0 = STANDARD_NTSC_BEAMCON;
#endif
		    
	dadd_head (&MDATA(cc_monitor)->modes, &this->node);
    }
    return (this);
}

static void
display_hires_view (view_t *v)
{
    if (this_data->current_view != v) {
	vdata_t *vd = VDATA(v);
	monitor_t *monitor = this_data->monitor;
	cop_t *cp = this_data->frames[F_STORE_LONG], *tmp;
	int depth = v->bitmap->depth, i;
	int hstart, hstop, vstart, vstop, j;
	int x, y, w = v->display.width, h = v->display.height;
 	u_word ddfstart, ddfwidth, con1;
	
 	/* round down to nearest even width */
 	/* w &= 0xfffe; */
	
	
 	/* calculate datafetch width. */
	
	ddfwidth = ((v->bitmap->bytes_per_row>>1)-2)<<2;		
	
	/* This will center the any overscanned display */
	/* and allow user to modify. */
	x = v->display.x + this_data->std_start_x - ((w - 640) >> 2);
	y = v->display.y + this_data->std_start_y - ((h - 200) >> 1);
	
	if (y&1)
	    y--;

	if (!(x&1)) 
 	    x--;
	
	hstart = x;
	hstop  = x+(w>>1);
	vstart = y;
	vstop  = y+h;
	ddfstart = (hstart-9) >> 1;

	/* check for hardware limits, AGA may allow more..? */              
	/* anyone got a 4000 I can borrow :^) -ch */                        
	if ((ddfstart&0xfffc) + ddfwidth > 0xd8) {
	    int d=0;

	    /* XXX anyone know the equality properties of intermixed logial AND's */
	    /* XXX and arithmetic operators? */
	    while (((ddfstart&0xfffc)+ddfwidth-d) > 0xd8) {
		d++;
	    }

	    ddfstart -= d;                                                  
	    hstart -= d<<1;                                                 
	    hstop -= d<<1;                                                  
	}                                                                   
	
	/* correct the datafetch to proper limits. */                       
	/* delay the actual display of the data until we need it. */        
	ddfstart &= 0xfffc;
	con1 = ((hstart-9)-(ddfstart<<1)) | (((hstart-9)-(ddfstart<<1))<<4);
	
	if (this_data->current_view) {
 	    VDATA(this_data->current_view)->flags &= ~VF_DISPLAY; /* mark as no longer */
								  /* displayed. */
	}
	this_data->current_view = v;

	cp = this_data->frames[F_STORE_LONG];
#if defined GRF_ECS
	tmp = find_copper_inst (cp, CI_MOVE(R_BEAMCON0));
	tmp->cp.inst.operand = this_data->beamcon0;
	tmp = find_copper_inst (cp, CI_MOVE(R_DIWHIGH));
	tmp->cp.inst.operand = CALC_DIWHIGH (hstart, vstart, hstop, vstop);
#endif /* ECS */
	tmp = find_copper_inst (cp, CI_MOVE(R_BPLCON0));
	tmp->cp.inst.operand = this_data->bplcon0 | ((depth & 0x7) << 12);
	tmp = find_copper_inst (cp, CI_MOVE(R_BPLCON1));
	tmp->cp.inst.operand = con1;
	tmp = find_copper_inst (cp, CI_MOVE(R_DIWSTART));
	tmp->cp.inst.operand = ((vstart & 0xff) << 8) | (hstart & 0xff);
	tmp = find_copper_inst (cp, CI_MOVE(R_DIWSTOP));
	tmp->cp.inst.operand = ((vstop & 0xff) << 8)  | (hstop & 0xff); 
	tmp = find_copper_inst (cp, CI_MOVE(R_DDFSTART));
	tmp->cp.inst.operand = ddfstart; 
	tmp = find_copper_inst (cp, CI_MOVE(R_DDFSTOP));
	tmp->cp.inst.operand = ddfstart + ddfwidth;

	tmp = find_copper_inst (cp, CI_MOVE(R_BPL0PTH));
	for (i = 0, j = 0; i < depth; j+=2, i++) {
	    /* update the plane pointers */
	    tmp[j].cp.inst.operand = HIADDR (PREP_DMA_MEM (v->bitmap->plane[i]));
	    tmp[j+1].cp.inst.operand = LOADDR (PREP_DMA_MEM (v->bitmap->plane[i]));
	}

	/* set mods correctly. */
	tmp = find_copper_inst (cp, CI_MOVE(R_BPL1MOD));
	tmp[0].cp.inst.operand = v->bitmap->row_mod;
	tmp[1].cp.inst.operand = v->bitmap->row_mod;

	/* set next pointers correctly */
	tmp = find_copper_inst (cp, CI_MOVE(R_COP1LCH));
	tmp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (this_data->frames[F_STORE_LONG]));
	tmp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (this_data->frames[F_STORE_LONG]));

	cp = this_data->frames[F_LONG];
	this_data->frames[F_LONG] = this_data->frames[F_STORE_LONG];
	this_data->frames[F_STORE_LONG] = cp;

	vd->flags |= VF_DISPLAY;

	cc_use_colormap (v, vd->colormap);
    }
    cc_load_mode (this);
}

static view_t *
get_current_view (dmode_t *d)
{
    return (this_data->current_view);
}


static void
hires_mode_vbl_handler (dmode_t *d)
{
    u_word vp = ((custom.vposr & 0x0007) << 8) | ((custom.vhposr) >> 8);

    if (vp < 12) {
   	custom.cop1lc = PREP_DMA_MEM (this_data->frames[F_LONG]);
	custom.copjmp1 = 0;
    }
}

#endif /* GRF_NTSC */
