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
#if defined (GRF_A2024)

#include "grf_cc_priv.h"
#include "../../../amiga/cc_2024.h"

#if defined (AMIGA_TEST)
#define DEBUG
#include <debug.h>
#else
#define D(x)
#endif

static void a2024_mode_vbl_handler (dmode_t *d);
static void display_a2024_view (view_t *v);
static view_t *get_current_view (dmode_t *d);

/* -------
 * |0 |1 |
 * |------
 * |2 |3 |
 * -------
 */

#define QUAD0_ID 0x0001
#define QUAD1_ID 0x00f1
#define QUAD2_ID 0x0f01
#define QUAD3_ID 0x0ff1

/* display order Q0 -> Q2 -> Q1 -> Q3 ---> */

enum frame_numbers {
    F_QUAD0, F_QUAD1, F_QUAD2, F_QUAD3,
    F_STORE_QUAD0, F_STORE_QUAD1, F_STORE_QUAD2, F_STORE_QUAD3,
    F_TOTAL
};

static dmode_t a2024_mode;
static dmdata_t a2024_mode_data;
static cop_t *a2024_frames[F_TOTAL];
static u_byte *hedley_init;			  /* init bitplane. */
static dmode_t *this;
static dmdata_t *this_data;

dmode_t *
cc_init_ntsc_a2024 (void)
{
    /* this function should only be called once. */
    if (!this) {
	int i;
	u_word len = std_a2024_copper_list_len;
	cop_t *cp;
	
	this = &a2024_mode;
	this_data = &a2024_mode_data;
	bzero (this, sizeof (dmode_t));
	bzero (this_data, sizeof (dmdata_t));

	this->name = "ntsc: A2024 15khz";
	this->nominal_size.width = 1024;
	this->nominal_size.height = 800;
	this_data->max_size.width = 1024;
	this_data->max_size.height = 800;
	this_data->min_size.width = 1024;
	this_data->min_size.height = 800;
	this_data->min_depth = 1;
	this_data->max_depth = 2;
	this->data = this_data;

	this->get_monitor = cc_get_monitor;
	this->alloc_view = cc_alloc_view;
	this->get_current_view = get_current_view;

	this_data->use_colormap = cc_a2024_use_colormap;
	this_data->get_colormap = cc_a2024_get_colormap;
	this_data->display_view = display_a2024_view;
	this_data->alloc_colormap = cc_a2024_alloc_colormap;
	this_data->monitor = cc_monitor;

	this_data->flags |= DMF_HEDLEY_EXP;

	this_data->frames = a2024_frames;
	this_data->frames[F_QUAD0] = alloc_chipmem (std_a2024_copper_list_size*F_TOTAL);
	if (!this_data->frames[F_QUAD0]) {
	    panic ("couldn't get chipmem for copper list");
	}

	/* setup the hedley init bitplane. */
	hedley_init = alloc_chipmem (128);
	if (!hedley_init) {
	    panic ("couldn't get chipmem for hedley init bitplane");
	}	
	for (i = 1; i < 128; i++) hedley_init[i] = 0xff;
	hedley_init[0] = 0x03;

	/* copy image of standard copper list. */
	bcopy (std_a2024_copper_list, this_data->frames[0], std_a2024_copper_list_size);

	/* set the init plane pointer. */
	cp = find_copper_inst (this_data->frames[F_QUAD0], CI_MOVE(R_BPL0PTH));
	cp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (hedley_init));
	cp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (hedley_init));

	for (i = 1; i < F_TOTAL; i++) {	    
	    this_data->frames[i] = &this_data->frames[i-1][len];
	    bcopy (this_data->frames[0], this_data->frames[i], std_a2024_copper_list_size);
	}	
	
	this_data->bplcon0 = 0x8200;		  /* hires */
	this_data->vbl_handler = (vbl_handler_func *) a2024_mode_vbl_handler;
	
		    
	dadd_head (&MDATA(cc_monitor)->modes, &this->node);
    }
    return (this);
}

static void
display_a2024_view (view_t *v)
{
    if (this_data->current_view != v) {
	vdata_t *vd = VDATA(v);
	monitor_t *monitor = this_data->monitor;
	cop_t *cp, *tmp;
	u_byte *inst_plane[2];
	u_byte **plane = inst_plane;
	u_long full_line = v->bitmap->bytes_per_row+v->bitmap->row_mod;
	u_long half_plane = full_line * v->bitmap->rows / 2;

	int line_mod = 0xbc;			  /* standard 2024 15khz mod. */
	int depth = v->bitmap->depth, i, j;

	plane[0] = v->bitmap->plane[0];
	if (depth == 2) {
	    plane[1] = v->bitmap->plane[1];
	}

	if (this_data->current_view) {
	    VDATA(this_data->current_view)->flags &= ~VF_DISPLAY; /* mark as no longer displayed. */
	}

	cp = this_data->frames[F_STORE_QUAD0];
	tmp = find_copper_inst (cp, CI_MOVE(R_COLOR1F));
	tmp = find_copper_inst (tmp, CI_MOVE(R_BPLCON0)); /* grab third one. */
	tmp->cp.inst.operand = this_data->bplcon0 | ((depth & 0x7) << 13); /* times 2 */
#if 0
	/* change display wait if 2 bitplanes. */
	if (depth == 2) {
	    tmp = find_copper_inst (cp, CI_WAIT(0, 43));
	    if (tmp) {
		tmp->cp.inst.opcode = CI_WAIT (0,42);
	    }
	} else {
	    /* XXX does same thing as above */
	    tmp = find_copper_inst (cp, CI_WAIT(0, 43));
	    if (tmp) {
		tmp->cp.inst.opcode = CI_WAIT (0, 42);
	    }
#if 0						  /* blows away the coplc data load :^) */
	    if (depth == 1) {
		tmp = find_copper_inst (cp, CI_MOVE(R_BPL2PTH));
		CWAIT (tmp, 255,255);
		CWAIT (tmp, 255,255);
	    }
#endif
	}

	/* set new modulos FIX: doesn't support larger bitmaps. */
	tmp = find_copper_inst (cp, CI_MOVE(R_BPLMOD1));
	tmp[0].cp.inst.operand = 0xbc + v->bitmap->row_mod;
	tmp[1].cp.inst.operand = 0xbc + v->bitmap->row_mod;
#endif
	
	bcopy (this_data->frames[F_STORE_QUAD0], this_data->frames[F_STORE_QUAD1], std_a2024_copper_list_size);
	bcopy (this_data->frames[F_STORE_QUAD0], this_data->frames[F_STORE_QUAD2], std_a2024_copper_list_size);
	bcopy (this_data->frames[F_STORE_QUAD0], this_data->frames[F_STORE_QUAD3], std_a2024_copper_list_size);

#define HALF_2024_LINE (512>>3)

	plane[0]--;
	plane[0]--;
	if (depth == 2) {
	    plane[1]--;
	    plane[1]--;
	}
	
	/*
	 * Set bitplane pointers.
	 */
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD0], CI_MOVE (R_BPLMOD2));
	CBUMP(tmp);
	CMOVE (tmp, R_BPL0PTH, HIADDR (PREP_DMA_MEM (&plane[0][0])));
	CMOVE (tmp, R_BPL0PTL, LOADDR (PREP_DMA_MEM (&plane[0][0])));
	CMOVE (tmp, R_BPL1PTH, HIADDR (PREP_DMA_MEM (&plane[0][full_line])));
	CMOVE (tmp, R_BPL1PTL, LOADDR (PREP_DMA_MEM (&plane[0][full_line])));
	if (depth == 2) {
	    CMOVE (tmp, R_BPL2PTH, HIADDR (PREP_DMA_MEM (&plane[1][0])));
	    CMOVE (tmp, R_BPL2PTL, LOADDR (PREP_DMA_MEM (&plane[1][0])));
	    CMOVE (tmp, R_BPL3PTH, HIADDR (PREP_DMA_MEM (&plane[1][full_line])));
	    CMOVE (tmp, R_BPL3PTL, LOADDR (PREP_DMA_MEM (&plane[1][full_line])));
	}
	
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD1], CI_MOVE (R_BPLMOD2));
	CBUMP(tmp);
	CMOVE (tmp, R_BPL0PTH, HIADDR (PREP_DMA_MEM (&plane[0][HALF_2024_LINE])));
	CMOVE (tmp, R_BPL0PTL, LOADDR (PREP_DMA_MEM (&plane[0][HALF_2024_LINE])));
	CMOVE (tmp, R_BPL1PTH, HIADDR (PREP_DMA_MEM (&plane[0][full_line+HALF_2024_LINE])));
	CMOVE (tmp, R_BPL1PTL, LOADDR (PREP_DMA_MEM (&plane[0][full_line+HALF_2024_LINE])));
	if (depth == 2) {
	    CMOVE (tmp, R_BPL2PTH, HIADDR (PREP_DMA_MEM (&plane[1][HALF_2024_LINE])));
	    CMOVE (tmp, R_BPL2PTL, LOADDR (PREP_DMA_MEM (&plane[1][HALF_2024_LINE])));
	    CMOVE (tmp, R_BPL3PTH, HIADDR (PREP_DMA_MEM (&plane[1][full_line+HALF_2024_LINE])));
	    CMOVE (tmp, R_BPL3PTL, LOADDR (PREP_DMA_MEM (&plane[1][full_line+HALF_2024_LINE])));
	}
	
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD2], CI_MOVE (R_BPLMOD2));
	CBUMP(tmp);
	CMOVE (tmp, R_BPL0PTH, HIADDR (PREP_DMA_MEM (&plane[0][half_plane])));
	CMOVE (tmp, R_BPL0PTL, LOADDR (PREP_DMA_MEM (&plane[0][half_plane])));
	CMOVE (tmp, R_BPL1PTH, HIADDR (PREP_DMA_MEM (&plane[0][half_plane+full_line])));
	CMOVE (tmp, R_BPL1PTL, LOADDR (PREP_DMA_MEM (&plane[0][half_plane+full_line])));
	if (depth == 2) {
	    CMOVE (tmp, R_BPL2PTH, HIADDR (PREP_DMA_MEM (&plane[1][half_plane])));
	    CMOVE (tmp, R_BPL2PTL, LOADDR (PREP_DMA_MEM (&plane[1][half_plane])));
	    CMOVE (tmp, R_BPL3PTH, HIADDR (PREP_DMA_MEM (&plane[1][half_plane+full_line])));
	    CMOVE (tmp, R_BPL3PTL, LOADDR (PREP_DMA_MEM (&plane[1][half_plane+full_line])));
	}
	
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD3], CI_MOVE (R_BPLMOD2));
	CBUMP(tmp);
	CMOVE (tmp, R_BPL0PTH, HIADDR (PREP_DMA_MEM (&plane[0][half_plane+HALF_2024_LINE])));
	CMOVE (tmp, R_BPL0PTL, LOADDR (PREP_DMA_MEM (&plane[0][half_plane+HALF_2024_LINE])));
	CMOVE (tmp, R_BPL1PTH, HIADDR (PREP_DMA_MEM (&plane[0][half_plane+full_line+HALF_2024_LINE])));
	CMOVE (tmp, R_BPL1PTL, LOADDR (PREP_DMA_MEM (&plane[0][half_plane+full_line+HALF_2024_LINE])));
	if (depth == 2) {
	    CMOVE (tmp, R_BPL2PTH, HIADDR (PREP_DMA_MEM (&plane[1][half_plane+HALF_2024_LINE])));
	    CMOVE (tmp, R_BPL2PTL, LOADDR (PREP_DMA_MEM (&plane[1][half_plane+HALF_2024_LINE])));
	    CMOVE (tmp, R_BPL3PTH, HIADDR (PREP_DMA_MEM (&plane[1][half_plane+full_line+HALF_2024_LINE])));
	    CMOVE (tmp, R_BPL3PTL, LOADDR (PREP_DMA_MEM (&plane[1][half_plane+full_line+HALF_2024_LINE])));
	}

	/*
	 * Mark Id's
	 */
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD1], CI_WAIT (126,21));
	CBUMP(tmp);
	CMOVE (tmp, R_COLOR01, QUAD1_ID);
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD2], CI_WAIT (126,21));
	CBUMP(tmp);
	CMOVE (tmp, R_COLOR01, QUAD2_ID);
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD3], CI_WAIT (126,21));
	CBUMP(tmp);
	CMOVE (tmp, R_COLOR01, QUAD3_ID);

	/* set next pointers correctly */
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD0], CI_MOVE (R_COP1LCH));
	tmp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (this_data->frames[F_STORE_QUAD1]));
	tmp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (this_data->frames[F_STORE_QUAD1]));
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD1], CI_MOVE (R_COP1LCH));
	tmp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (this_data->frames[F_STORE_QUAD2]));
	tmp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (this_data->frames[F_STORE_QUAD2]));
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD2], CI_MOVE (R_COP1LCH));
	tmp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (this_data->frames[F_STORE_QUAD3]));
	tmp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (this_data->frames[F_STORE_QUAD3]));
	tmp = find_copper_inst (this_data->frames[F_STORE_QUAD3], CI_MOVE (R_COP1LCH));
	tmp[0].cp.inst.operand = HIADDR (PREP_DMA_MEM (this_data->frames[F_STORE_QUAD0]));
	tmp[1].cp.inst.operand = LOADDR (PREP_DMA_MEM (this_data->frames[F_STORE_QUAD0]));

	/* swap new pointers in. */
	for (i = F_STORE_QUAD0, j = F_QUAD0;
	     i <= F_STORE_QUAD3; i++, j++) {
	    cp = this_data->frames[j];
	    this_data->frames[j] = this_data->frames[i];
	    this_data->frames[i] = cp;
	}

	this_data->current_view = v;
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
a2024_mode_vbl_handler (dmode_t *d)
{
    u_word vp = ((custom.vposr & 0x0007) << 8) | ((custom.vhposr) >> 8);

    if (vp < 12) {
	custom.cop1lc = PREP_DMA_MEM (this_data->frames[this_data->hedley_current]);
	custom.copjmp1 = 0;
    }
    this_data->hedley_current++;
    this_data->hedley_current &= 0x3;		  /* if 4 then 0. */
}

#endif /* GRF_A2024 */

