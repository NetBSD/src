/*	$NetBSD: vmparam.h,v 1.1 2003/02/03 23:09:29 matt Exp $	*/

/*-
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MACHINE_VMPARAM_H_
#define _MACHINE_VMPARAM_H_

#define	USRSTACK	VM_MAXUSER_ADDRESS

#ifndef	MAXTSIZ
#define	MAXTSIZ		(64*1024*1024)		/* max text size */
#endif

#ifndef	DFLDSIZ
#define	DFLDSIZ		(128*1024*1024)		/* default data size */
#endif

#ifndef	MAXDSIZ
#define	MAXDSIZ		(1*1024*1024*1024)	/* max data size */
#endif

#ifndef	DFLSSIZ
#define	DFLSSIZ		(2*1024*1024)		/* default stack size */
#endif

#ifndef	MAXSSIZ
#define	MAXSSIZ		(32*1024*1024)		/* max stack size */
#endif

/*
 * Size of shared memory map
 */
#ifndef	SHMMAXPGS
#define	SHMMAXPGS	1024
#endif

/*
 * Size of User Raw I/O map
 */
#define	USRIOSIZE	1024

#if 1
/*
 * These definitions give you a 2GB kernel VA space.  The other set matches
 * the way other PPC ports lay out their 256MB kernel address space.
 */
#define	VM_MIN_ADDRESS		((vaddr_t)0)
#define	VM_MAXUSER_ADDRESS	((vaddr_t)0xffff0000-NBPG)
#define	VM_MAX_ADDRESS		VM_MAXUSER_ADDRESS
#define	VM_MIN_KERNEL_ADDRESS	((vaddr_t)0x80000000)
#define	VM_MAX_KERNEL_ADDRESS	((vaddr_t)0xff000000)
#else
/*
 * Would like to have MAX addresses = 0, but this doesn't (currently) work
 */
#define	VM_MIN_ADDRESS		((vaddr_t)0)
#define	VM_MAXUSER_ADDRESS	((vaddr_t)0x80000000-NBPG)
#define	VM_MAX_ADDRESS		VM_MAXUSER_ADDRESS
#define	VM_MIN_KERNEL_ADDRESS	((vaddr_t)(KERNEL_SR << ADDR_SR_SHFT))
#define	VM_MAX_KERNEL_ADDRESS	(VM_MIN_KERNEL_ADDRESS + SEGMENT_LENGTH - 1)
#endif

/* XXX max. amount of KVM to be used by buffers. */
#ifndef VM_MAX_KERNEL_BUF
#define VM_MAX_KERNEL_BUF	(SEGMENT_LENGTH * 7 / 10)

/*
 * Override the default pager_map size, there's not enough KVA.
 */
#define PAGER_MAP_SIZE		(4 * 1024 * 1024)
#endif

#define	VM_PHYS_SIZE		(USRIOSIZE * NBPG)

#define	__HAVE_PMAP_PHYSSEG

struct pmap_physseg {
	struct pv_entry *pvent;
	char *attrs;
};

#define VM_PHYSSEG_MAX		16	/* 1? */
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH
#define VM_PHYSSEG_NOADD		/* can't add RAM after vm_mem_init */

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#endif /* _MACHINE_VMPARAM_H_ */
