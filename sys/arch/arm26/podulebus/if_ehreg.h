/*	$NetBSD: if_ehreg.h,v 1.1.2.2 2000/12/08 09:26:23 bouyer Exp $	*/

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
#define EH_CTRL_IE	0x01	/* Interrupt enable */
#define EH_CTRL_MEDIA	0x02	/* Media select (0 = 10b2, 1 = 10bT) */
#define EH_CTRL_LINK	0x02	/* Link beat detect */

#endif
