/*      $NetBSD: pmap.h,v 1.22 1998/01/03 00:28:43 thorpej Exp $     */

/* 
 * Copyright (c) 1987 Carnegie-Mellon University
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * Changed for the VAX port. /IC
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	@(#)pmap.h	7.6 (Berkeley) 5/10/91
 */


#ifndef	PMAP_H
#define	PMAP_H

#include <machine/mtpr.h>

struct pte;

/*
 * Pmap structure
 *  pm_stack holds lowest allocated memory for the process stack.
 */

typedef struct pmap {
	vm_offset_t		 pm_stack; /* Base of alloced p1 pte space */
	int                      ref_count;   /* reference count        */
	struct pte		*pm_p0br; /* page 0 base register */
	long			 pm_p0lr; /* page 0 length register */
	struct pte		*pm_p1br; /* page 1 base register */
	long			 pm_p1lr; /* page 1 length register */
} *pmap_t;

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */

typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	struct pte	*pv_pte;	/* pte for this physical page */
} *pv_entry_t;

/* ROUND_PAGE used before vm system is initialized */
#define ROUND_PAGE(x)   (((uint)(x) + PAGE_SIZE-1)& ~(PAGE_SIZE - 1))
#define	TRUNC_PAGE(x)	((uint)(x) & ~(PAGE_SIZE - 1))

/* Mapping macros used when allocating SPT */
#define	MAPVIRT(ptr, count)					\
	(vm_offset_t)ptr = virtual_avail;			\
	virtual_avail += (count) * NBPG;

#define	MAPPHYS(ptr, count, perm)				\
	(vm_offset_t)ptr = avail_start + KERNBASE;		\
	avail_start += (count) * NBPG;

#ifdef	_KERNEL

extern	struct pmap kernel_pmap_store;

#define	pmap_kernel()			(&kernel_pmap_store)

#endif	/* _KERNEL */

/* Routines that are best to define as macros */
#define	pmap_copy(a,b,c,d,e) 		/* Dont do anything */
#define	pmap_update()	mtpr(0,PR_TBIA)	/* Update buffes */
#define	pmap_pageable(a,b,c,d)		/* Dont do anything */
#define	pmap_collect(pmap)		/* No need so far */
#define	pmap_reference(pmap)	if(pmap) (pmap)->ref_count++
#define	pmap_phys_address(phys) ((u_int)(phys)<<PAGE_SHIFT)
#define	pmap_is_referenced(phys)	(FALSE)
#define	pmap_clear_reference(pa)	pmap_page_protect(pa, VM_PROT_NONE)
#define pmap_change_wiring(pmap, v, w)  /* no need */
#define	pmap_remove(pmap, start, slut)  pmap_protect(pmap, start, slut, 0)

/* These can be done as efficient inline macros */
#define pmap_copy_page(src, dst)				\
	__asm__("addl3 $0x80000000,%0,r0;addl3 $0x80000000,%1,r1;	\
	    movc3 $1024,(r0),(r1)"				\
	    :: "r"(src),"r"(dst):"r0","r1","r2","r3","r4","r5");

#define pmap_zero_page(phys)					\
	__asm__("addl3 $0x80000000,%0,r0;movc5 $0,(r0),$0,$1024,(r0)" \
	    :: "r"(phys): "r0","r1","r2","r3","r4","r5");

/* Prototypes */
void	pmap_bootstrap __P((void));

void	pmap_pinit __P((pmap_t));

struct proc;
void	pmap_activate __P((struct proc *));
void	pmap_deactivate __P((struct proc *));

#endif PMAP_H
