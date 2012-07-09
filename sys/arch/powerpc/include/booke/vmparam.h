/*	$NetBSD: vmparam.h,v 1.6 2012/07/09 17:55:15 matt Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _POWERPC_BOOKE_VMPARAM_H_
#define _POWERPC_BOOKE_VMPARAM_H_

/*
 * Most of the definitions in this can be overriden by a machine-specific
 * vmparam.h if required.  Otherwise a port can just include this file
 * get the right thing to happen.
 */

/*
 * BookE processors have 4K pages.  Override the PAGE_* definitions to be
 * compile-time constants.
 */
#define	PAGE_SHIFT	12
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

#ifndef	USRSTACK
#define	USRSTACK	VM_MAXUSER_ADDRESS
#endif

#ifndef	MAXTSIZ
#define	MAXTSIZ		(2*256*1024*1024)	/* maximum text size */
#endif

#ifndef	MAXDSIZ
#define	MAXDSIZ		(13*256*1024*1024U)	/* maximum data size */
#endif

#ifndef	MAXSSIZ
#define	MAXSSIZ		(1*256*1024*1024-PAGE_SIZE) /* maximum stack size */
#endif

#ifndef	DFLDSIZ
#define	DFLDSIZ		(256*1024*1024)		/* default data size */
#endif

#ifndef	DFLSSIZ
#define	DFLSSIZ		(2*1024*1024)		/* default stack size */
#endif

/*
 * Default number of pages in the user raw I/O map.
 */
#ifndef USRIOSIZE
#define	USRIOSIZE	1024
#endif

/*
 * The number of seconds for a process to be blocked before being
 * considered very swappable.
 */
#ifndef MAXSLP
#define	MAXSLP		20
#endif

/*
 * Some system constants
 */

#define	VM_MIN_ADDRESS		((vaddr_t) 0)
#define	VM_MAXUSER_ADDRESS	((vaddr_t) /* -32768 */ 0xffff8000)
#define	VM_MAX_ADDRESS		VM_MAXUSER_ADDRESS
#define	VM_MIN_KERNEL_ADDRESS	((vaddr_t) 0xe4000000)
#define	VM_MAX_KERNEL_ADDRESS	((vaddr_t) 0xfefff000)

#define	VM_PHYSSEG_STRAT	VM_PSTRAT_BIGFIRST

#ifndef VM_PHYS_SIZE
#define	VM_PHYS_SIZE		(USRIOSIZE * PAGE_SIZE)
#endif

#include <common/pmap/tlb/vmpagemd.h>

#endif /* _POWERPC_BOOKE_VMPARAM_H_ */
