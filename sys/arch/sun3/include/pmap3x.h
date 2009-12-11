/*	$NetBSD: pmap3x.h,v 1.27 2009/12/11 13:52:57 tsutsui Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper.
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

/*
 * Physical map structures exported to the VM code.
 * XXX - Does user-level code really see this struct?
 */

#include <sys/simplelock.h>

struct pmap {
	struct a_tmgr_struct	*pm_a_tmgr; 	/* Level-A table manager */
	u_long              	pm_a_phys;  	/* MMU level-A phys addr */
	struct simplelock	pm_lock;    	/* lock on pmap */
	int             	pm_refcount;	/* reference count */
	int             	pm_version;
};

#ifdef _KERNEL
/* Common function for pmap_resident_count(), pmap_wired_count() */
segsz_t pmap_count(pmap_t, int);

/* This needs to be a macro for kern_sysctl.c */
#define	pmap_resident_count(pmap)	(pmap_count((pmap), 0))

/* This needs to be a macro for vm_mmap.c */
#define	pmap_wired_count(pmap)  	(pmap_count((pmap), 1))

/* We use the PA plus some low bits for device mmap. */
#define pmap_phys_address(addr) 	(addr)

#define	pmap_update(pmap)		/* nothing (yet) */

/* Map a given physical region to a virtual region */
vaddr_t pmap_map(vaddr_t, paddr_t, paddr_t, int);

static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

/*
 * Flags to tell pmap_enter `this is not to be cached', etc.
 * Since physical addresses are always aligned, we can use
 * the low order bits for this.
 */
#define	PMAP_OBMEM	0x00	/* unused */
#define	PMAP_OBIO	0x00	/* unused */
#define	PMAP_VME16	0x10	/* pmap will add the necessary offset */
#define	PMAP_VME32	0x20	/* etc. */
#define	PMAP_NC		0x40	/* tells pmap_enter to set PTE_CI */
#define	PMAP_SPEC	0xFF	/* mask to get all above. */

/* MMU specific segment size */
#define	SEGSHIFT	19	        /* LOG2(NBSG) */
#define	NBSG		(1 << SEGSHIFT)	/* bytes/segment */
#define	SEGOFSET	(NBSG - 1)	/* byte offset into segment */

#define	sun3x_round_seg(x)	((((vaddr_t)(x)) + SEGOFSET) & ~SEGOFSET)
#define	sun3x_trunc_seg(x)	((vaddr_t)(x) & ~SEGOFSET)
#define	sun3x_seg_offset(x)	((vaddr_t)(x) & SEGOFSET)
 
#endif	/* _KERNEL */
