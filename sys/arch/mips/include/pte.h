/*	$NetBSD: pte.h,v 1.2 1997/06/15 17:24:22 mhitch Exp $	*/

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef  __MIPS_PTE_H__
#define  __MIPS_PTE_H__


#if defined(MIPS1) && defined(MIPS3)
#error  Cannot yet support both  "MIPS1" (r2000 family) and "MIPS3" (r4000 family) in the same kernel.
#endif

#ifndef MIPS3
#include <mips/mips1_pte.h>
#endif

#ifdef MIPS3
#include <mips/mips3_pte.h>
#endif


#if defined(_KERNEL) && !defined(_LOCORE)
/*
 * Kernel virtual address to page table entry and visa versa.
 */
#define	kvtopte(va) \
	(Sysmap + (((vm_offset_t)(va) - VM_MIN_KERNEL_ADDRESS) >> PGSHIFT))
#define	ptetokv(pte) \
	((((pt_entry_t *)(pte) - Sysmap) << PGSHIFT) + VM_MIN_KERNEL_ADDRESS)

extern	pt_entry_t *Sysmap;		/* kernel pte table */
extern	u_int Sysmapsize;		/* number of pte's in Sysmap */
#endif	/* defined(_KERNEL) && !defined(_LOCORE) */

#endif /* __MIPS_PTE_H__ */
