/*	$NetBSD: pmap.h,v 1.14.10.2 2008/01/09 01:46:24 matt Exp $	*/

/*	$OpenBSD: pmap.h,v 1.14 2001/05/09 15:31:24 art Exp $	*/

/*
 * Copyright (c) 1998,1999 Michael Shalayeff
 * All rights reserved.
 *
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
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/*
 * Copyright 1996 1995 by Open Software Foundation, Inc.   
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/* 
 * Copyright (c) 1990,1993,1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: pmap.h 1.24 94/12/14$
 *	Author: Mike Hibler, Bob Wheeler, University of Utah CSL, 9/90
 */

/*
 *	Pmap header for hppa.
 */

#ifndef	_HPPA_PMAP_H_
#define	_HPPA_PMAP_H_

#include <sys/simplelock.h>
#include <machine/pte.h>

typedef
struct pmap {
	TAILQ_ENTRY(pmap)	pmap_list;	/* pmap free list */
	struct simplelock	pmap_lock;	/* lock on map */
	int			pmap_refcnt;	/* reference count */
	pa_space_t		pmap_space;	/* space for this pmap */
	u_int			pmap_pid;	/* protection id for pmap */
	struct pmap_statistics	pmap_stats;	/* statistics */
} *pmap_t;
extern pmap_t	kernel_pmap;			/* The kernel's map */

#ifdef _KERNEL

#define PMAP_NC		0x100

#define cache_align(x)	(((x) + dcache_line_mask) & ~(dcache_line_mask))
extern int dcache_line_mask;

#define	PMAP_STEAL_MEMORY	/* we have some memory to steal */

/*
 * according to the parisc manual aliased va's should be
 * different by high 12 bits only.
 */
#define	PMAP_PREFER(o,h,s,td)	do {					\
	vaddr_t pmap_prefer_hint;					\
	pmap_prefer_hint = (*(h) & HPPA_PGAMASK) | ((o) & HPPA_PGAOFF);	\
	if (pmap_prefer_hint < *(h))					\
		pmap_prefer_hint += HPPA_PGALIAS;			\
	*(h) = pmap_prefer_hint;					\
} while(0)

#define pmap_kernel_va(VA)	\
	(((VA) >= VM_MIN_KERNEL_ADDRESS) && ((VA) <= VM_MAX_KERNEL_ADDRESS))

#define pmap_kernel()			(kernel_pmap)
#define	pmap_resident_count(pmap)	((pmap)->pmap_stats.resident_count)
#define pmap_wired_count(pmap)		((pmap)->pmap_stats.wired_count)
#define pmap_reference(pmap) \
do { if (pmap) { \
	simple_lock(&pmap->pmap_lock); \
	pmap->pmap_refcnt++; \
	simple_unlock(&pmap->pmap_lock); \
} } while (0)
#define pmap_collect(pmap)
#define pmap_release(pmap)
#define pmap_copy(dpmap,spmap,da,len,sa)
#define	pmap_update(p)
void	pmap_activate(struct lwp *);

static __inline void
pmap_deactivate(struct lwp *l)
{
}

#define pmap_phys_address(x)	((x) << PGSHIFT)
#define pmap_phys_to_frame(x)	((x) >> PGSHIFT)

static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

static __inline int
pmap_prot(struct pmap *pmap, vm_prot_t prot)
{
	extern u_int kern_prot[], user_prot[];

	return (pmap == kernel_pmap ? kern_prot : user_prot)[prot];
}

#define	pmap_sid(pmap, va) \
	((((va) & 0xc0000000) != 0xc0000000) ? \
	 (pmap)->pmap_space : HPPA_SID_KERNEL)

#endif /* _KERNEL */

#endif /* _HPPA_PMAP_H_ */
