/* $NetBSD: sfbvar.h,v 1.5 1998/10/22 01:03:08 briggs Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <machine/sfbreg.h>
#include <dev/rcons/raster.h>
#include <dev/wscons/wscons_raster.h>

extern int	sfb_cnattach __P((tc_addr_t));

struct sfb_devconfig;
struct fbcmap;
struct fbcursor;
struct fbcurpos;

struct sfb_devconfig {
	vaddr_t dc_vaddr;		/* memory space virtual base address */
	paddr_t dc_paddr;		/* memory space physical base address */
	psize_t dc_size;		/* size of slot memory */

	int	dc_wid;			/* width of frame buffer */
	int	dc_ht;			/* height of frame buffer */
	int   	dc_depth;		/* depth, bits per pixel */
	int	dc_rowbytes;		/* bytes in a FB scan line */

	vaddr_t	dc_videobase;		/* base of flat frame buffer */

	struct	raster dc_raster;	/* raster description */
	struct	rcons  dc_rcons;	/* raster blitter control info */

	int	dc_blanked;		/* currently has video disabled */

	int	dc_cmap_red[256];
	int	dc_cmap_green[256];
	int	dc_cmap_blue[256];

	short	dc_curpos_x;
	short	dc_curpos_y;

	short	dc_cursor_enable;

	int	dc_cursor_red[3];
	int	dc_cursor_green[3];
	int	dc_cursor_blue[3];

	char	dc_cursor_bitmap[1024];
};
	
struct sfb_softc {
	struct	device sc_dev;

	struct	sfb_devconfig *sc_dc;	/* device configuration */

	int	nscreens;
};
