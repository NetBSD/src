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
 *	$Id: grf_view.h,v 1.2 1994/01/29 07:00:05 chopps Exp $
 */

#if ! defined (_GRF_VIEW_H)
#define _GRF_VIEW_H

/* removes the view if displaying */
typedef void remove_view_func (view_t *v);                               

/* free's the memory for this view and removes it if displaying */
typedef void free_view_func (view_t *v);                               

/* displays this view */
typedef void display_view_func (view_t *v);             

/* returns mode of this view */                  
typedef dmode_t *get_mode_func (view_t *v);    

/* fills in color map structure with requested registers */
typedef int get_colormap_func (view_t *v, colormap_t *);

/* sets this views colors according to the color map structure. */
typedef int use_colormap_func (view_t *v, colormap_t *);

struct view {
    bmap_t  *bitmap;				  /* bitmap. */
    box_t    display;				  /* viewable area. */
    void    *data;				  /* view specific data. */

    /* functions */
    display_view_func *display_view;		  /* make this view active */
    remove_view_func  *remove_view;		  /* remove this view if active */
    free_view_func    *free_view;		  /* free this view */
    get_mode_func     *get_display_mode;	  /* get the mode this view belongs to */
    get_colormap_func *get_colormap;		  /* get a color map for registers */
    use_colormap_func *use_colormap;		  /* use color map to load registers */
};

view_t * grf_alloc_view (dmode_t *d, dimen_t *dim, u_byte depth);
void grf_display_view (view_t *v);
void grf_remove_view (view_t *v);
void grf_free_view (view_t *v);
dmode_t *grf_get_display_mode (view_t *v);
int grf_get_colormap (view_t *v, colormap_t *cm);
int grf_use_colormap (view_t *v, colormap_t *cm);

#define VDISPLAY_LINE(v, p, l) ((v)->bitmap->plane[p] +\
	(((v)->bitmap->bytes_per_row+(v)->bitmap->row_mod)*l))

#endif /* _GRF_VIEW_H */

