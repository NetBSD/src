/*	$NetBSD: tc_machdep.h,v 1.1.8.2 2007/07/11 20:02:55 mjf Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Jonathan Stone, Chris G. Demetriou
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
 * Machine-specific definitions for TURBOchannel support.
 *
 * This file must typedef the following types:
 *
 *	tc_addr_t	TURBOchannel bus address
 *	tc_offset_t	TURBOchannel bus address difference (offset)
 *
 * This file must prototype or define the following functions
 * or macros (one or more of which may be no-ops):
 *
 *	tc_mb()		read/write memory barrier (any CPU<->memory
 *			reads/writes before must complete before any
 *			CPU<->memory reads/writes after).
 *	tc_wmb()	write memory barrier (any CPU<->memory writes
 *			before must complete before any CPU<->memory
 *			writes after).
 *	tc_syncbus()	sync TC bus; make sure CPU writes are
 *			propagated across the TURBOchannel bus.
 *	tc_badaddr()	return non-zero if the given address is invalid.
 *	TC_DENSE_TO_SPARSE()
 *			convert the given physical address in
 *			TURBOchannel dense space to the corresponding
 *			address in TURBOchannel sparse space.
 *	TC_PHYS_TO_UNCACHED()
 *			convert the given system memory physical address
 *			to the physical address of the corresponding
 *			region that is not cached.
 */

#ifndef _VAX_TC_MACHDEP_H_
#define _VAX_TC_MACHDEP_H_

typedef uint32_t	tc_addr_t;
typedef uint32_t	tc_offset_t;

#define	tc_mb()		do { } while (/*CONSTCOND*/ 0)
#define	tc_wmb()	do { } while (/*CONSTCOND*/ 0)
#define	tc_syncbus()	do { } while (/*CONSTCOND*/ 0)

#define	tc_badaddr(tcaddr) badaddr((void *)(tcaddr), sizeof (uint32_t))

#define	TC_DENSE_TO_SPARSE(addr)  (addr)
#define	TC_PHYS_TO_UNCACHED(addr) (addr)

/*
 * Use the following macros to compare device names on a vax, as
 * the autoconfig structs are in a state of flux.
 */
#define TC_BUS_MATCHNAME(ta, name) \
	(strncmp( (ta)->ta_modname, (name), TC_ROM_LLEN+1) == 0)

#endif /* _VAX_TC_MACHDEP_H_ */
