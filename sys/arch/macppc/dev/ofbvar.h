/*	$NetBSD: ofbvar.h,v 1.1 1998/10/14 12:15:13 tsubai Exp $	*/

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

struct ofb_devconfig {
	paddr_t	dc_paddr;		/* physcal address */
	int	dc_ih;			/* ihandle of this node */

	struct raster	dc_raster;	/* raster description */
	struct rcons	dc_rcons;	/* raster blitter control info */
};

#define dc_width	dc_raster.width
#define dc_height	dc_raster.height
#define dc_depth	dc_raster.depth
#define dc_linebytes	dc_raster.linelongs * sizeof(u_int32_t)

	
struct ofb_softc {
	struct	device sc_dev;

	struct	ofb_devconfig *sc_dc;	/* device configuration */
	int nscreens;

	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
};
