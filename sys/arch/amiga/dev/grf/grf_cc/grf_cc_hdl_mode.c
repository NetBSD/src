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
 *	$Id: grf_cc_hdl_mode.c,v 1.2 1994/01/29 07:00:22 chopps Exp $
 */

#if defined (GRF_A2024) && defined (GRF_NTSC)

#include "grf_cc_priv.h"
#include "../../../amiga/cc_2024.h"

#if defined (AMIGA_TEST)
#define DEBUG
#include <debug.h>
#else
#define D(x)
#endif

static void hires_dlace_mode_vbl_handler (dmode_t *d);
static void display_hires_dlace_view (view_t *v);
static view_t *get_current_view (dmode_t *d);

enum frame_numbers {
    F_LONG,
    F_SHORT,
    F_STORE_LONG,
    F_STORE_SHORT,
    F_TOTAL
};

static dmode_t hires_dlace_mode;
static dmdata_t hires_dlace_mode_data;
static cop_t *hires_dlace_frames[F_TOTAL];
static dmode_t *this;
static dmdata_t *this_data;

dmode_t *
cc_init_ntsc_hires_dlace (void)
{
    /* this function should only be called once. */
    if (!this) {
	u_word len = std_dlace_copper_list_len;
	cop_t *cp;
	
	this = &hires_dlace_mode;
	this_data = &hires_dlace_mode_data;
	bzero (this, sizeof (dmode_t));
	bzero (this_data, sizeof (dmdata_t));
	
	this->name = "ntsc: hires double interlace";
	this->nominal_size.width = 640;
	this->nominal_size.height = 800;
	this_data->max_size.width = 724;
	this_data->max_size.height = 800;
	this_data->min_size.width = 320;
	this_data->min_size.height = 400;
	this_data->min_depth = 1;
	this_data->max_depth = 2;
	this->data = this_data;

	this->get_monitor = cc_get_monitor;
	this->alloc_view = cc_alloc_view;
	this->get_current_view = get_current_view;

	this_data->use_colormap = cc_a2024_use_colormap;
	this_data->get_colormap = cc_a2024_get_colormap;
        this_data->alloc_colormap = cc_a2024_alloc_colormap;
        this_data->display_view = display_hires_dlace_view;
	this_data->monitor = cc_monitor;

	this_data->flags |= DMF_INTERLACE;

	this_data->frames = hires_dlace_frames;
	this_data->frames[F_LONG] = alloc_chipmem (std_dlace_copper_list_size*F_TOTAL);
	if (!this_data->frames[F_LONG]) {
	    panic ("couldn't get chipmem for copper list");
	}
	
	this_data->frames[F_SHORT] = &this_data->frames[F_LONG][len];
	this_data->frames[F_STORE_LONG] = &this_data->frames[F_SHORT][len];
	this_data->frames[F_STORE_SHORT] = &this_data->frames[F_STORE_LONG][len];
	
	bcopy (std_dlace_copper_list, this_data->frames[F_STORE_LONG], std_dlace_copper_list_size);
	bcopy (std_dlace_copper_list, this_data->frames[F_STORE_SHORT], std_dlace_copper_list_size);
	bcopy (std_dlace_copper_list, this_data->frames[F_LONG], std_dlace_copper_list_size);
	bcopy (std_dlace_copper_list, this_data->frames[F_SHORT], std_dlace_copper_list_size);
	
	this_data->bplcon0 = 0x8204;		  /* hires, color composite enable, dlace. */
	this_data->std_start_x = STANDARD_VIEW_X;
	this_data->std_start_y = STANDARD_VIEW_Y;
	this_data->vbl_handler = (vbl_handler_func *) hires_dlace_mode_vbl_handler;
#if defined (GRF_ECS)	
	this_data->beamcon0 = STANDARD_NTSC_BEAMCON;
#endif
	dadd_head (&MDATA(cc_monitor)->modes, &this->node);
    }
    return (this);
}

static void
display_hires_dlace_view (view_t *v)
{
    if (this_data->current_view != v) {
	vdata_t *vd = VDATA(v);
	monitor_t *monitor = this_data->monitor;
	cop_t *cp = this_data->frames[F_STORE_LONG], *tmp;
	int depth = v->bitmap->depth, i;
	int hstart, hstop, vstart, vstop, j;
	int x, y, w = v->display.width, h = v->display.height;
	u_word ddfstart, ddfwidth, con1;
	u_word mod1l, mod2l;

	/* round down to nearest even width */
	/* w &= 0xfffe; */
	
	/* calculate datafetch width. */

	ddfwidth = ((v->bitmap->bytes_per_row>>1)-2)<<2;		

	/* This will center the any overscanned display */
	/* and allow user to modify. */
	x = v->display.x + this_data->std_start_x - ((w - 640) >> 2);
	y = v->display.y + this_data->std_start_y - ((h - 800) >> 3);

	if (y&1)
	    y--;
	
	if (!(x&1)) 
	    x--;
	
	hstart = x;
	hstop  = x+(w>>1);
	vstart = y;
	vstop  = y+(h>>2);
	
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
	tmp->cp.inst.operand = this_data->bplcon0 | ((depth & 0x7) << 13); /* times two. */
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

	mod1l = v->bitmap->bytes_per_row + v->bitmap->row_mod;
	mod2l = mod1l << 1;

	/* update plane pointers. */
	tmp = find_copper_inst (cp, CI_MOVE(R_BPL0PTH));
	tmp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (&v->bitmap->plane[0][0]));
	tmp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (&v->bitmap->plane[0][0]));
	tmp[2].cp.inst.operand = HIADDR (PREP_DMA_MEM (&v->bitmap->plane[0][mod1l]));
	tmp[3].cp.inst.operand = LOADDR (PREP_DMA_MEM (&v->bitmap->plane[0][mod1l]));
	if (depth == 2) {
	    tmp[4].cp.inst.operand = HIADDR (PREP_DMA_MEM (&v->bitmap->plane[1][0]));
	    tmp[5].cp.inst.operand = LOADDR (PREP_DMA_MEM (&v->bitmap->plane[1][0]));
	    tmp[6].cp.inst.operand = HIADDR (PREP_DMA_MEM (&v->bitmap->plane[1][mod1l]));
	    tmp[7].cp.inst.operand = LOADDR (PREP_DMA_MEM (&v->bitmap->plane[1][mod1l]));
	}
	
	/* set modulos. */
	tmp = find_copper_inst (cp, CI_MOVE(R_BPL1MOD));
	tmp[0].cp.inst.operand = mod2l + mod1l;
	tmp[1].cp.inst.operand = mod2l + mod1l;


	/* set next coper list pointers */
        tmp = find_copper_inst (cp, CI_MOVE(R_COP1LCH));
        tmp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (this_data->frames[F_STORE_SHORT]));
        tmp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (this_data->frames[F_STORE_SHORT]));

	bcopy (this_data->frames[F_STORE_LONG], this_data->frames[F_STORE_SHORT],
	       std_dlace_copper_list_size);

	/* these are the only ones that are different from long frame. */
	cp = this_data->frames[F_STORE_SHORT];	
	/* update plane pointers. */
	tmp = find_copper_inst (cp, CI_MOVE(R_BPL0PTH));
	tmp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (&v->bitmap->plane[0][mod2l]));
	tmp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (&v->bitmap->plane[0][mod2l]));
	tmp[2].cp.inst.operand = HIADDR (PREP_DMA_MEM (&v->bitmap->plane[0][mod2l+mod1l]));
	tmp[3].cp.inst.operand = LOADDR (PREP_DMA_MEM (&v->bitmap->plane[0][mod2l+mod1l]));
	if (depth == 2) {
	    tmp[4].cp.inst.operand = HIADDR (PREP_DMA_MEM (&v->bitmap->plane[1][mod2l]));
	    tmp[5].cp.inst.operand = LOADDR (PREP_DMA_MEM (&v->bitmap->plane[1][mod2l]));
	    tmp[6].cp.inst.operand = HIADDR (PREP_DMA_MEM (&v->bitmap->plane[1][mod2l+mod1l]));
	    tmp[7].cp.inst.operand = LOADDR (PREP_DMA_MEM (&v->bitmap->plane[1][mod2l+mod1l]));
	}

	/* set next copper list pointers */
        tmp = find_copper_inst (cp, CI_MOVE(R_COP1LCH));
        tmp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (this_data->frames[F_STORE_LONG]));
        tmp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (this_data->frames[F_STORE_LONG]));

	cp = this_data->frames[F_LONG];
	this_data->frames[F_LONG] = this_data->frames[F_STORE_LONG];
	this_data->frames[F_STORE_LONG] = cp;

	cp = this_data->frames[F_SHORT];
	this_data->frames[F_SHORT] = this_data->frames[F_STORE_SHORT];
	this_data->frames[F_STORE_SHORT] = cp;
      
	vd->flags |= VF_DISPLAY;
	cc_a2024_use_colormap (v, vd->colormap);
    }
    cc_load_mode (this);
}

static view_t *
get_current_view (dmode_t *d)
{
    return (this_data->current_view);
}

static void
hires_dlace_mode_vbl_handler (dmode_t *d)
{
    u_word vp = ((custom.vposr & 0x0007) << 8) | ((custom.vhposr) >> 8);

    if (vp < 12) {
    	if (custom.vposr & 0x8000) {
	    custom.cop1lc = PREP_DMA_MEM (this_data->frames[F_LONG]);
   	} else {
	    custom.cop1lc = PREP_DMA_MEM (this_data->frames[F_SHORT]);
    	}
	custom.copjmp1 = 0;
    }
}

#endif /* GRF_A2024 && GRF_NTSC */
