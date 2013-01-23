/*	$NetBSD: pmap.h,v 1.24.2.1 2013/01/23 00:05:58 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_MACHINE_PMAP_H
#define	_MACHINE_PMAP_H

#ifdef _KERNEL
/*
 * Physical map structures exported to the VM code.
 */

struct pmap {
	unsigned char   	*pm_segmap; 	/* soft copy of segmap */
	int             	pm_ctxnum;	/* MMU context number */
	u_int             	pm_refcount;	/* reference count */
	int             	pm_version;
};

/*
 * We give the pmap code a chance to resolve faults by
 * reloading translations that it was forced to unload.
 * This function does that, and calls vm_fault if it
 * could not resolve the fault by reloading the MMU.
 */
int _pmap_fault(struct vm_map *, vaddr_t, vm_prot_t);

/* This lets us have some say in choosing VA locations. */
extern void pmap_prefer(vaddr_t, vaddr_t *, int);
#define PMAP_PREFER(fo, ap, sz, td) pmap_prefer((fo), (ap), (td))

/* This needs to be a macro for kern_sysctl.c */
extern segsz_t pmap_resident_pages(pmap_t);
#define	pmap_resident_count(pmap)	(pmap_resident_pages(pmap))

/* This needs to be a macro for vm_mmap.c */
extern segsz_t pmap_wired_pages(pmap_t);
#define	pmap_wired_count(pmap)	(pmap_wired_pages(pmap))

/* We use the PA plus some low bits for device mmap. */
#define pmap_phys_address(addr) 	(addr)

#define	pmap_update(pmap)		/* nothing (yet) */

/* Map a given physical region to a virtual region */
extern vaddr_t pmap_map(vaddr_t, paddr_t, paddr_t, int);

/* Extract the PMEG for a given physical address. */
extern int _pmap_extract_pmeg(pmap_t, vaddr_t);

static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

/*
 * Since PTEs also contain type bits, we have to have some way
 * to tell pmap_enter `this is an IO page' or `this is not to
 * be cached'.  Since physical addresses are always aligned, we
 * can do this with the low order bits.
 *
 * The values below must agree with pte.h such that:
 *	(PMAP_OBIO << PG_MOD_SHIFT) == PGT_OBIO
 */
#define	PMAP_OBMEM	0x00
#define	PMAP_OBIO	0x04	/* tells pmap_enter to use PG_OBIO */
#define	PMAP_MBMEM	0x08	/* etc (sun-2/120) */
#define	PMAP_MBIO	0x0C	/* etc (sun-2/120) */
#define	PMAP_VME0	0x08	/* etc (sun-2/50) */
#define	PMAP_VME8	0x0C	/* etc (sun-2/50) */
#define	PMAP_NC		0x00	/* tells pmap_enter to set PG_NC */
#define	PMAP_SPEC	0x0C	/* mask to get all above. */

void pmap_procwr(struct proc *, vaddr_t, size_t);

#endif	/* _KERNEL */

/* MMU specific segment value */
#define	SEGSHIFT	15	        /* LOG2(NBSG) */
#define	NBSG		(1 << SEGSHIFT)	/* bytes/segment */
#define	SEGOFSET	(NBSG - 1)	/* byte offset into segment */

#define	sun2_round_seg(x)	((((vaddr_t)(x)) + SEGOFSET) & ~SEGOFSET)
#define	sun2_trunc_seg(x)	((vaddr_t)(x) & ~SEGOFSET)
#define	sun2_seg_offset(x)	((vaddr_t)(x) & SEGOFSET)

#endif	/* _MACHINE_PMAP_H */
