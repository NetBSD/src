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

#if ! defined (_GRF_CCPRIV_H)
#define _GRF_CCPRIV_H

#include "../../../amiga/cc.h"
#include "../../../amiga/cc_chipmem.h"
#include "../../../amiga/cc_copper.h"
#include "../../../amiga/cc_registers.h"
#include "../../../amiga/dlists.h"
#include "../grf_types.h"
#include "../grf_bitmap.h"
#include "../grf_view.h"
#include "../grf_mode.h"
#include "../grf_monitor.h"
#include "../grf_colormap.h"
#include "../grf_draw.h"
#include "grf_cc_custom.h"

typedef colormap_t *alloc_colormap_func (int);

typedef struct monitor_data {
    dll_list_t  modes;				  /* a list of supported modes. */
    dmode_t *current_mode;
    u_long    flags;				  /* monitor flags. */
} mdata_t;

typedef struct display_mode_data {
    monitor_t  * monitor;			  /* the monitor that owns this mode. */
    view_t * current_view;			  /* current view to be displayed. */
    cop_t  **frames;
    u_word    hedley_current;			  /* current hedley quadrent. */
    u_word    bplcon0;				  /* bplcon0 data. */
    u_word    std_start_x;
    u_word    std_start_y;
#if defined (GRF_ECS)
    /* ECS registers. */
    u_word   beamcon0;
    u_word   hbstart;				  /* no modes use the rest of these */
    u_word   hbstop;				  /*    ECS registers. */
    u_word   hsstart;
    u_word   hsstop;
    u_word   vbstart;
    u_word   vbstop;
    u_word   vsstart;
    u_word   vsstop;
#endif
    /* some limit stuff. */
    dimen_t   max_size;				  /* largest fit. */
    dimen_t   min_size;				  /* smallest fit. */
    u_word   min_depth;
    u_word   max_depth;
    u_long   flags;				  /* mode specific flags. */
    use_colormap_func *use_colormap;
    get_colormap_func *get_colormap;
    alloc_colormap_func *alloc_colormap;
    display_view_func  *display_view;
    vbl_handler_func   *vbl_handler;		  /* gets called every vertical blank. */
						  /* when this is the current mode.*/
} dmdata_t;

enum dmode_flag_bits {
    DMB_INTERLACE,
    DMB_HEDLEY_EXP
};

enum dmode_flags {
    DMF_INTERLACE = 1 << DMB_INTERLACE,
    DMF_HEDLEY_EXP = 1 << DMB_HEDLEY_EXP
};

typedef struct view_data {
    dmode_t *mode;				  /* the mode for this view */
    colormap_t *colormap;
    u_long    flags;				  /* view specific flags. */
} vdata_t;

enum view_flag_bits {
    VB_DISPLAY,
};

enum view_flags {
    VF_DISPLAY = 1 << VB_DISPLAY,		  /* set if view is being displayed */
};

/* mode locally public functions */
int cc_init_modes (void);
#if defined (GRF_NTSC)
#if defined (GRF_A2024)
dmode_t *cc_init_ntsc_a2024 (void);
dmode_t *cc_init_ntsc_hires_dlace (void);
#endif
dmode_t *cc_init_ntsc_hires_lace (void);
dmode_t *cc_init_ntsc_hires (void);
#endif
#if defined (GRF_PAL)
#if defined (GRF_A2024)
dmode_t *cc_init_pal_a2024 (void);
dmode_t *cc_init_pal_hires_dlace (void);
#endif
dmode_t *cc_init_pal_hires (void);
dmode_t *cc_init_pal_hires_lace (void);
#endif

monitor_t *cc_get_monitor (dmode_t *d);

view_t *cc_get_current_view (dmode_t *m);
view_t *cc_alloc_view   (dmode_t *mode, dimen_t *dim, u_byte depth);
void cc_mode_vbl_handler (dmode_t *m);
void cc_init_view    (view_t *v, bmap_t *bm, dmode_t *mode, box_t *dbox);
void cc_free_view    (view_t *v);
int cc_use_colormap (view_t *v, colormap_t *);
int cc_get_colormap (view_t *v, colormap_t *);
void cc_remove_view  (view_t *v);
void cc_load_mode (dmode_t *d);
colormap_t * cc_alloc_colormap (int);

#if defined (GRF_A2024)
int cc_a2024_use_colormap (view_t *v, colormap_t *);
int cc_a2024_get_colormap (view_t *v, colormap_t *);
colormap_t * cc_a2024_alloc_colormap (int);

extern cop_t std_a2024_copper_list[];
extern int std_a2024_copper_list_len;
extern int std_a2024_copper_list_size;

extern cop_t std_dlace_copper_list[];
extern int std_dlace_copper_list_len;
extern int std_dlace_copper_list_size;

#endif /* GRF_A2024 */

extern cop_t std_copper_list[];
extern int std_copper_list_len;
extern int std_copper_list_size;
extern monitor_t *cc_monitor;

#define MDATA(m) ((mdata_t *)(m->data))
#define DMDATA(d) ((dmdata_t *)(d->data))
#define VDATA(v) ((vdata_t *)(v->data))
#define RWDATA(r) ((rwdata_t *)(r->data))

#if defined (GRF_ECS)
#define CALC_DIWHIGH(hs, vs, he, ve) \
        ((u_word)((he&0x100)<<5)|(ve&0x700)|((hs&0x100)>>3)|((vs&0x700)>>8))
#endif

#endif /* _GRF_CCPRIV_H */
