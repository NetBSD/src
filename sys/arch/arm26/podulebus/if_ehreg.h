/*	$NetBSD: if_ehreg.h,v 1.2 2000/12/16 18:23:14 bjh21 Exp $	*/

/*
 * This file is in the public domain
 */

#ifndef _IF_EHREG_H
#define _IF_EHREG_H

/*
 * Register definitions for i-cubed EtherLan 100-, 200-, and 500-series cards.
 */

/* Loosely derived from Linux drivers/acorn/net/etherh.c. */

/* All offsets in bus_size units */
#define EH_DP8390	0x000	/* MEMC space */
#define EH_DATA		0x200	/* MEMC space */
#define EH_CTRL		0x200	/* FAST space */

/* Bits of the control register */
#define EH_CTRL_IE	0x01	/* Interrupt enable (W) */
#define EH_CTRL_IS	0x01	/* Interrupt status (R) */
#define EH_CTRL_MEDIA	0x02	/* Media select (0 = 10b2, 1 = 10bT) (W) */
#define EH_CTRL_LINK	0x02	/* Link beat detect (R) */

#endif
