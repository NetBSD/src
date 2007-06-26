/*	$NetBSD: if_snvar.h,v 1.19.10.1 2007/06/26 18:12:53 garbled Exp $	*/

/*
 * Copyright (c) 1991   Algorithmics Ltd (http://www.algor.co.uk)
 * You may use, copy, and modify this program so long as you retain the
 * copyright line.
 */

/*
 * if_snvar.h -- National Semiconductor DP8393X (SONIC) NetBSD/mac68k vars
 */

/*
 * Vendor types
 */
#define	SN_VENDOR_UNKNOWN	0xff	/* Unknown */
#define	SN_VENDOR_APPLE		0x00	/* Apple Computer/compatible */
#define	SN_VENDOR_DAYNA		0x01	/* Dayna/Kinetics EtherPort */
#define	SN_VENDOR_APPLE16	0x02	/* Apple Twisted Pair NB */
#define	SN_VENDOR_ASANTELC	0x09	/* Asante Macintosh LC Ethernet */

void	sn_get_enaddr(bus_space_tag_t, bus_space_handle_t, bus_size_t,
		      uint8_t *);
