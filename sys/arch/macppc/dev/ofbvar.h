/*	$NetBSD: ofbvar.h,v 1.8.6.1 2006/04/22 11:37:41 simonb Exp $	*/

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

#ifndef _OFBVAR_H_
#define _OFBVAR_H_

struct ofb_softc {
	struct	device sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;

	u_int32_t sc_addrs[30];		/* "assigned-addresses" storage */
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	
	int sc_node, sc_ih, sc_mode;
	paddr_t sc_fbaddr;
	
	struct vcons_data vd;
};

#endif /* _OFBVAR_H_ */
