/*	$NetBSD: pmap.h,v 1.1 2001/03/29 04:44:20 fredette Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * NB:  The details of struct pmap are exposed ONLY when
 * building a kernel.  LKMs and user-level programs see
 * only this anonymous declaration.  Note that the actual
 * declaration may vary on different m68k kernels.
 */
struct pmap;
typedef struct pmap *pmap_t;

/*
 * Physical map structures exported to the VM code.
 * XXX - Does user-level code really see this struct?
 */

struct pmap {
	unsigned char   	*pm_segmap; 	/* soft copy of segmap */
	int             	pm_ctxnum;	/* MMU context number */
	simple_lock_data_t	pm_lock;    	/* lock on pmap */
	int             	pm_refcount;	/* reference count */
	int             	pm_version;
};

#ifdef _KERNEL
extern	struct pmap	kernel_pmap_store;
#define	pmap_kernel()	(&kernel_pmap_store)

/*
 * We give the pmap code a chance to resolve faults by
 * reloading translations that it was forced to unload.
 * This function does that, and calls vm_fault if it
 * could not resolve the fault by reloading the MMU.
 */
int _pmap_fault __P((vm_map_t, vm_offset_t, vm_prot_t));

/* This lets us have some say in choosing VA locations. */
extern void pmap_prefer(vm_offset_t, vm_offset_t *);
#define PMAP_PREFER(fo, ap) pmap_prefer((fo), (ap))

/* This needs to be a macro for kern_sysctl.c */
extern segsz_t pmap_resident_pages(pmap_t);
#define	pmap_resident_count(pmap)	(pmap_resident_pages(pmap))

/* This needs to be a macro for vm_mmap.c */
extern segsz_t pmap_wired_pages(pmap_t);
#define	pmap_wired_count(pmap)	(pmap_wired_pages(pmap))

/* We use the PA plus some low bits for device mmap. */
#define pmap_phys_address(addr) 	(addr)

/* Our memory is contiguous (or nearly so). */
#define pmap_page_index(pa) (atop(pa))

/* Map a given physical region to a virtual region */
extern vm_offset_t pmap_map __P((vm_offset_t, vm_offset_t, vm_offset_t, int));

/*
 * Since PTEs also contain type bits, we have to have some way
 * to tell pmap_enter `this is an IO page' or `this is not to
 * be cached'.  Since physical addresses are always aligned, we
 * can do this with the low order bits.
 *
 * The values below must agree with pte.h such that:
 *	(PMAP_OBIO << PG_MOD_SHIFT) == PGT_OBIO
 */
#define	PMAP_OBIO	0x04	/* tells pmap_enter to use PG_OBIO */
#define	PMAP_VME16	0x08	/* etc */
#define	PMAP_MBMEM	0x08	/* etc (sun-2/120) */
#define	PMAP_MBIO	0x0C	/* etc (sun-2/120) */
/* 0x0C is also used for ??? on the 2/50 */
#define	PMAP_NC		0x00	/* tells pmap_enter to set PG_NC */
#define	PMAP_SPEC	0x0C	/* mask to get all above. */

#endif	/* _KERNEL */

void pmap_procwr __P((struct proc *, vaddr_t, size_t));

#endif	/* _MACHINE_PMAP_H */
