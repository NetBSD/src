/*	$NetBSD: grfabs_reg.h,v 1.1.1.1 1995/03/26 07:12:15 leo Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
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
 */

#ifndef _GRFABS_REG_H
#define _GRFABS_REG_H

struct point {
    long	x;
    long	y;
};
typedef struct point point_t;

struct dimension {
    u_long	width;
    u_long	height;
};
typedef struct dimension dimen_t;

struct box {
    long	x;
    long	y;
    u_long	width;
    u_long	height;
};
typedef struct box box_t;

struct rectangle {
    long	left;
    long	top;
    long	right;
    long	bottom;
};
typedef struct rectangle rect_t;

typedef struct bitmap bmap_t;
typedef struct colormap colormap_t;
typedef struct display_mode dmode_t;

/*
 * View stuff.
 */

struct view {
    bmap_t	*bitmap;	/* bitmap.				*/
    box_t	display;	/* viewable area.			*/
    dmode_t	*mode;		/* the mode for this view		*/
    colormap_t	*colormap;	/* the colormap for this view		*/
    int		flags;
};
typedef struct view view_t;

/* View-flags: */
#define	VF_DISPLAY	1	/* view is on the air now		*/

/*
 * Bitmap stuff.
 */
struct bitmap {
    u_short	bytes_per_row;	/* number of bytes per display row.	*/
    u_short	rows;		/* number of display rows.		*/
    u_short	depth;		/* depth of bitmap.			*/
    u_char	*plane;		/* plane data for bitmap.		*/
    u_char	*hw_address;	/* mappable bitplane pointer.		*/
};

/*
 * Colormap stuff.
 */
struct colormap {
    u_short	nentries;	/* number of entries in the map		*/
    u_short	*centry;	/* actual colormap			*/
};

/*
 * Create a colormap entry
 */
#define	MAKE_CENTRY(r,g,b)	(((r & 0xf)<<8)|((g & 0xf)<<4)|(b & 0xf))
#define	MAX_CENTRIES		256		/* that all there is	*/

/* display mode */
struct display_mode {
    LIST_ENTRY(display_mode)	link;
    u_char			*name;		/* logical name for mode. */
    dimen_t			size;		/* screen size		  */
    u_char			depth;		/* screen depth		  */
    u_short			vm_reg;		/* video mode register	  */
    view_t			*current_view;	/* view displaying me	  */
};

/*
 * Misc draw related macros.
 */
#define INIT_BOX(b,xx,yy,ww,hh)						\
	do {								\
		(b)->x = xx;						\
		(b)->y = yy;						\
		(b)->width = ww;					\
		(b)->height = hh;					\
	} while(0)

/*
 * Prototypes:
 */
view_t	*grf_alloc_view __P((dmode_t *d, dimen_t *dim, u_char depth));
void	grf_display_view __P((view_t *v));
void	grf_remove_view __P((view_t *v));
void	grf_free_view __P((view_t *v));
int	grf_get_colormap __P((view_t *, colormap_t *));
int	grf_use_colormap __P((view_t *, colormap_t *));

#endif /* _GRFABS_REG_H */
