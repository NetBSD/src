/*	$NetBSD: if_ehreg.h,v 1.1.2.3 2001/01/05 17:34:04 bouyer Exp $	*/

/*
 * This file is in the public domain
 */

#ifndef _IF_EHREG_H
#define _IF_EHREG_H

/*
 * Register definitions for i-cubed EtherLan 100-, 200-, and 500-series cards.
 */

/*
 * Addr		r/w	Descr
 * SYNC+0x0000	r	ROM data
 * SYNC+0x0000	w	ROM page latch
 * FAST+0x0800	r/w	AUX1
 * FAST+0x2800	r/w	AUX2
 * MEMC+0x0000	r/w	MX98902A registers
 * MEMC+0x0200	r/w	MX98902A data
 *
 * AUX1 bits:
 * 0 r Interrupt status
 * 0 w Interrupt enable
 * 1 r (E100) Link beat (0 = link, 1 = no link)
 * 1 w (E100) Media select (0 = 10b2, 1 = 10bT)
 * 1 r (E200) MAU ID input (from MAU)
 * 1 w (E200) MAU ID output (to MAU)
 *
 * AUX2 bits:
 * 0 r Media sense (0 = 10bT, 1 = 10b2)
 */

/* Loosely derived from Linux drivers/acorn/net/etherh.c. */

/* All offsets in bus_size units */
#define EH_DP8390	0x000	/* MEMC space */
#define EH_DATA		0x200	/* MEMC space */
#define EH_CTRL		0x200	/* FAST space */
#define EH_CTRL2	0xa00	/* FAST space */

/* Bits of the control register */
#define EH_CTRL_IE	0x01	/* Interrupt enable (W) */
#define EH_CTRL_IS	0x01	/* Interrupt status (R) */
#define EH_CTRL_MEDIA	0x02	/* Media select (0 = 10b2, 1 = 10bT) (W) */
#define EH_CTRL_NOLINK	0x02	/* Link beat detect (R) */

/*
 * The EtherLan 200 is strange.  A write to AUX1 following another write has
 * the effect described above, but a write following a read affects this:
 */
#define EH200_CTRL_MAU	0x02	/* MAU input/output */

/* Bits of control register 2 */
#define EH_CTRL2_10B2	0x01	/* Media sense (0 = 10bT, 1 = 10b2) (R) */

/* EtherLan 200 MAU types */
#define EH200_MAUID_10_T	2
#define EH200_MAUID_10_2	3
#define EH200_MAUID_10_5	4
#endif
