/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-softwre.com> of Allegro Networks, Inc.
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

#ifndef _POWERPC_OEA_VMPARAM_H_
#define _POWERPC_OEA_VMPARAM_H_

#include <sys/queue.h>

/*
 * Most of the definitions in this can be overriden by a machine-specific
 * vmparam.h if required.  Otherwise a port can just include this file
 * get the right thing to happen.
 */

#ifndef	USRSTACK
#define	USRSTACK		VM_MAXUSER_ADDRESS
#endif

#ifndef	USRSTACK32
#define	USRSTACK32		((uint32_t)VM_MAXUSER_ADDRESS)
#endif

#ifndef	MAXTSIZ
#define	MAXTSIZ			(64*1024*1024)		/* maximum text size */
#endif

#ifndef	MAXDSIZ
#define	MAXDSIZ			(1024*1024*1024)	/* maximum data size */
#endif

#ifndef	MAXSSIZ
#define	MAXSSIZ			(32*1024*1024)		/* maximum stack size */
#endif

#ifndef	DFLDSIZ
#define	DFLDSIZ			(128*1024*1024)		/* default data size */
#endif

#ifndef	DFLSSIZ
#define	DFLSSIZ			(2*1024*1024)		/* default stack size */
#endif

/*
 * Default maximum amount of shared memory pages
 */
#ifndef SHMMAXPGS
#define	SHMMAXPGS		1024
#endif

/*
 * Default number of pages in the user raw I/O map.
 */
#ifndef USRIOSIZE
#define	USRIOSIZE		1024
#endif

/*
 * The number of seconds for a process to be blocked before being
 * considered very swappable.
 */
#ifndef MAXSLP
#define	MAXSLP			20
#endif

/*
 * Segment handling stuff
 */
#define	SEGMENT_LENGTH	( 0x10000000L)
#define	SEGMENT_MASK	(~0x0fffffffL)

/*
 * Macros to manipulate VSIDs
 */
#if 0
/*
 * Move the SR# to the top 4 bits to make the lower 20 bits entirely random
 * so to give better PTE distribution.
 */
#define	VSID_MAKE(sr, hash)	(((sr) << (ADDR_SR_SHFT-4))|((hash) & 0xfffff))
#define	VSID_TO_SR(vsid)	(((vsid) >> (ADDR_SR_SHFT-4)) & 0xF)
#define	VSID_TO_HASH(vsid)	((vsid) & 0xfffff)
#define	VSID_SR_INCREMENT	0x00100000
#else
#define	VSID_MAKE(sr, hash)	((sr) | (((hash) & 0xfffff) << 4))
#define	VSID_TO_SR(vsid)	((vsid) & 0xF)
#define	VSID_TO_HASH(vsid)	(((vsid) >> 4) & 0xfffff)
#define	VSID_SR_INCREMENT	0x00000001
#endif

/*
 * Fixed segments
 */
#ifndef USER_SR
#define	USER_SR			12
#endif
#define	KERNEL_SR		13
#define	KERNEL2_SR		14
#define	KERNEL2_SEGMENT		VSID_MAKE(KERNEL2_SR, KERNEL_VSIDBITS)
#define	KERNEL_VSIDBITS		0xfffff
#define	KERNEL_SEGMENT		VSID_MAKE(KERNEL_SR, KERNEL_VSIDBITS)
#define	EMPTY_SEGMENT		VSID_MAKE(0, KERNEL_VSIDBITS)
#define	USER_ADDR		((void *)(USER_SR << ADDR_SR_SHFT))

/*
 * Some system constants
 */
#ifndef	NPMAPS
#define	NPMAPS		32768	/* Number of pmaps in system */
#endif

#define	VM_MIN_ADDRESS		((vaddr_t) 0)
#define	VM_MAXUSER_ADDRESS	((vaddr_t) ~0xfffL)
#define	VM_MAX_ADDRESS		VM_MAXUSER_ADDRESS
#define	VM_MIN_KERNEL_ADDRESS	((vaddr_t) (KERNEL_SR << ADDR_SR_SHFT))
#define	VM_MAX_KERNEL_ADDRESS	(VM_MIN_KERNEL_ADDRESS + 2*SEGMENT_LENGTH)

#ifndef VM_PHYSSEG_MAX
#define	VM_PHYSSEG_MAX		16
#endif
#define	VM_PHYSSEG_STRAT	VM_PSTRAT_BIGFIRST
#define	VM_PHYSSEG_NOADD

#ifndef VM_PHYS_SIZE
#define	VM_PHYS_SIZE		(USRIOSIZE * PAGE_SIZE)
#endif

#ifndef VM_MAX_KERNEL_BUF
#define	VM_MAX_KERNEL_BUF	(SEGMENT_LENGTH * 3 / 4)
#endif

#define	VM_NFREELIST		16	/* 16 distinct memory segments */
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_FIRST256	1
#define	VM_FREELIST_FIRST16	2
#define	VM_FREELIST_MAX		3

#ifndef _LOCORE

LIST_HEAD(pvo_head, pvo_entry);

#if __NetBSD_Version__ > 105180000
#define	__HAVE_VM_PAGE_MD

struct vm_page_md {
	struct pvo_head mdpg_pvoh;
	unsigned int mdpg_attrs; 
};

#define	VM_MDPAGE_INIT(pg) do {			\
	LIST_INIT(&(pg)->mdpage.mdpg_pvoh);	\
	(pg)->mdpage.mdpg_attrs = 0;		\
} while (/*CONSTCOND*/0)

#else

#define	__HAVE_PMAP_PHYSSEG

struct pmap_physseg {
	struct pvo_head *pvoh;
	char *attrs;
};

#endif

#endif	/* _LOCORE */

#endif /* _POWERPC_OEA_VMPARAM_H_ */
