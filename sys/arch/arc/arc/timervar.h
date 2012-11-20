/* $NetBSD: timervar.h,v 1.6.14.1 2012/11/20 03:01:01 tls Exp $ */
/* NetBSD: clockvar.h,v 1.4 1997/06/22 08:02:18 jonathan Exp  */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
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

/*
 * timerfns structure:
 *
 * function switch used by chip-independent timer code, to access
 * chip-dependent routines.
 */
struct timerfns {
	void	(*tf_init)(device_t);
};

extern uint32_t last_cp0_count;

void timerattach(device_t, const struct timerfns *);

#ifdef ENABLE_INT5_STATCLOCK
extern struct evcnt statclock_ev;
void statclockintr(struct clockframe *);
#endif
