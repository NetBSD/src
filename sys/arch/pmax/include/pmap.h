/* 
 * Copyright (c) 1987 Carnegie-Mellon University
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: @(#)pmap.h	7.6 (Berkeley) 2/4/93
 * $Id: pmap.h,v 1.2 1993/10/15 02:57:13 deraadt Exp $
 */

#ifndef	_PMAP_MACHINE_
#define	_PMAP_MACHINE_

/*
 * TLB hash table values.
 * SHIFT2 should shift virtual address bit 22 to the high bit of the index.
 *			address:	index:
 *	USRTEXT		0x00400000	10xxxxxxx
 *	USRDATA		0x10000000	00xxxxxxx
 *	USRSTACK	0x7FFFFFFF	11xxxxxxx
 * This gives 1/2 the table to data, 1/4 for text and 1/4 for stack.
 * Note: the current process has its hash table mapped at PMAP_HASH_UADDR.
 *	the kernel's hash table is mapped at PMAP_HASH_KADDR.
 *	The size of the hash table is known in locore.s.
 * The wired entries in the TLB will contain the following:
 *	UPAGES			(for curproc)
 *	PMAP_HASH_UPAGES	(for curproc)
 *	PMAP_HASH_KPAGES	(for kernel)
 * The kernel doesn't actually use a pmap_hash_t, the pm_hash field is NULL and
 * all the PTE entries are stored in a single array at PMAP_HASH_KADDR.
 * If we need more KPAGES that the TLB has wired entries, then we can switch
 * to a global pointer for the kernel TLB table.
 * If we try to use a hash table for the kernel, wired TLB entries become a
 * problem.
 * Note: PMAP_HASH_UPAGES should be a multiple of MACH pages (see pmap_enter()).
 */
#define PMAP_HASH_UPAGES	1
#define PMAP_HASH_KPAGES	5
#define PMAP_HASH_UADDR		(UADDR - PMAP_HASH_UPAGES * NBPG)
#define PMAP_HASH_KADDR		(UADDR - (PMAP_HASH_UPAGES + PMAP_HASH_KPAGES) * NBPG)
#define PMAP_HASH_NUM_ENTRIES	256
#define PMAP_HASH_SIZE_SHIFT	4
#define PMAP_HASH_SHIFT1	12
#define PMAP_HASH_SHIFT2	21
#define PMAP_HASH_MASK1		0x07f
#define PMAP_HASH_MASK2		0x080
#define PMAP_HASH_SIZE		(PMAP_HASH_NUM_ENTRIES*sizeof(struct pmap_hash))

/* compute pointer to pmap hash table */
#define PMAP_HASH(va) \
	((((va) >> PMAP_HASH_SHIFT1) & PMAP_HASH_MASK1) | \
	 (((va) >> PMAP_HASH_SHIFT2) & PMAP_HASH_MASK2))

/*
 * A TLB hash entry.
 */
typedef struct pmap_hash {
	struct {
		u_int	low;		/* The TLB low register value. */
		u_int	high;		/* The TLB high register value. */
	} pmh_pte[2];
} *pmap_hash_t;

/*
 * Machine dependent pmap structure.
 */
struct pmap {
	int			pm_count;	/* pmap reference count */
	simple_lock_data_t	pm_lock;	/* lock on pmap */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	int			pm_flags;	/* see below */
	int			pm_tlbpid;	/* address space tag */
	pmap_hash_t		pm_hash;	/* TLB cache */
	unsigned		pm_hash_ptes[PMAP_HASH_UPAGES];
};

typedef struct pmap *pmap_t;

#define PM_MODIFIED	1		/* flush tlbpid before resume() */

/*
 * Defines for pmap_attributes[phys_mach_page];
 */
#define PMAP_ATTR_MOD	0x01	/* page has been modified */
#define PMAP_ATTR_REF	0x02	/* page has been referenced */

#ifdef	KERNEL
extern struct pmap kernel_pmap_store;
extern pmap_t kernel_pmap;
extern	char *pmap_attributes;		/* reference and modify bits */
#endif	KERNEL
#endif	_PMAP_MACHINE_
