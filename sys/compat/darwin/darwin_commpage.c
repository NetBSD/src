/*	$NetBSD: darwin_commpage.c,v 1.1 2004/07/03 00:14:30 manu Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_commpage.c,v 1.1 2004/07/03 00:14:30 manu Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/lwp.h>
#include <sys/proc.h>

#include <machine/cpu.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h> 
#include <uvm/uvm.h>

#include <compat/darwin/darwin_commpage.h>

#include "opt_altivec.h"

static struct uvm_object *darwin_commpage_uao = NULL;

static void darwin_commpage_init(struct darwin_commpage *);

int
darwin_commpage_map(p) 
	struct proc *p;
{
	int error;
	vaddr_t kvaddr;
	vaddr_t pvaddr;
	size_t memsize;
	
	/* 
	 * XXX This crashes. For now we map only one page,
	 * to avoid the crash, but this ought to be fixed
	 */
#ifdef notyet
	memsize = round_page(sizeof(struct darwin_commpage));
#else
	memsize = PAGE_SIZE;
#endif

	if (darwin_commpage_uao == NULL) {
		darwin_commpage_uao = uao_create(memsize, 0);
		kvaddr = vm_map_min(kernel_map);

		error = uvm_map(kernel_map, &kvaddr, memsize,
		    darwin_commpage_uao, 0, PAGE_SIZE,
		    UVM_MAPFLAG(UVM_PROT_RWX, UVM_PROT_RWX,
		    UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
		if (error != 0) {
			uao_detach(darwin_commpage_uao);
			darwin_commpage_uao = NULL;
			return -1;
		}

		error = uvm_map_pageable(kernel_map, kvaddr,
		    kvaddr + memsize, FALSE, 0);
		if (error != 0) {
			uao_detach(darwin_commpage_uao);
			darwin_commpage_uao = NULL;
			return -1;
		}

		darwin_commpage_init((struct darwin_commpage *)kvaddr);
	}

	uao_reference(darwin_commpage_uao);
	pvaddr = DARWIN_COMMPAGE_BASE;

	if ((error = uvm_map(&p->p_vmspace->vm_map, &pvaddr,
	    memsize, darwin_commpage_uao, 0, 0,
	    UVM_MAPFLAG(UVM_PROT_RWX, UVM_PROT_RWX,
	    UVM_INH_SHARE, UVM_ADV_NORMAL, UVM_FLAG_FIXED))) != 0) {
#ifdef DEBUG_DARWIN
		printf("uvm_map darwin_commpage failed (error %d)\n", error);
#endif
		return -1;
	}

#ifdef DEBIG_DARWIN
	printf("mapped darwin_commpage at 0x%08lx\n", (long)pvaddr);
#endif

	return 0;
}


void
darwin_commpage_init(dcp)
	struct darwin_commpage *dcp;
{
	int ncpu, name[2];
	size_t sz;
	int error;
	int bcopy_glue[2];
	int pthread_self[3];

	/* 
	 * XXX Only one page is mapped yet (see higher in the file)
	 */
#ifdef notyet
	bzero(dcp, sizeof(*dcp));
#else
	bzero(dcp, PAGE_SIZE);
#endif
	dcp->dcp_version = DARWIN_COMMPAGE_VERSION;	

	name[0] = CTL_HW;
	name[1] = HW_NCPU;
	sz = sizeof(ncpu);

	error = old_sysctl(&name[0], 2, &ncpu, &sz, NULL, 0, NULL);
	if (error != 0)
		ncpu = 1; /* At least there should be one */

	dcp->dcp_ncpu = ncpu;
	dcp->dcp_cap |= (ncpu << DARWIN_CAP_NCPUSHIFT);

#ifdef ALTIVEC
	dcp->dcp_altivec = 1;
	dcp->dcp_cap |= DARWIN_CAP_ALTIVEC;
#endif

#ifdef _LP64
	dcp->dcp_64bit = 1; 
	dcp->dcp_cap |= DARWIN_CAP_64BIT;
#endif

#ifndef CACHELINESIZE
#define CACHELINESIZE 32	/* for i386... */
#endif
	dcp->dcp_cachelinelen = CACHELINESIZE;
#if (CACHELINESIZE == 32)
	dcp->dcp_cap |= DARWIN_CAP_CACHE32;
#elif (CACHELINESIZE == 64)
	dcp->dcp_cap |= DARWIN_CAP_CACHE64;
#elif (CACHELINESIZE == 128)
	dcp->dcp_cap |= DARWIN_CAP_CACHE128;
#endif

	dcp->dcp_2pow52 = 2^52;
	dcp->dcp_10pow6 = 10^6;

	/*
	 * On Darwin, these are maintained up to date by the kernel 
	 */
	dcp->dcp_timebase = 0; /* XXX */	
	dcp->dcp_timestamp = 0; /* XXX */	
	dcp->dcp_secpertick = hz; /* XXX Not sure */	

	/* dcp->dcp_mach_absolute_time */
	/* dcp->dcp_spinlock_try */
	/* dcp->dcp_spinlock_lock */
	/* dcp->dcp_spinlock_unlock */
	/* dcp->dcp_pthread_specific */
	/* dcp->dcp_gettimeofday */
	/* dcp->dcp_sys_dcache_flush */
	/* dcp->dcp_sys_icache_invalidate */

	/* 
	 * XXX hack until we write something better
	 */
#ifdef __powerpc__
	pthread_self[0] = 0x38007ff2; /* li r0,32754 */
	pthread_self[1] = 0x44000002; /* sc */
	pthread_self[2] = 0x4e800020; /* blr */
	memcpy(&dcp->dcp_pthread_self[0], 
	    (void *)pthread_self, sizeof(pthread_self));	
#endif

	/* dcp->dcp_spinlock_relinquish */

#ifdef __powerpc__
	bcopy_glue[0] = 0x38000000;	/* li r0,0 */
	bcopy_glue[1] = 0x7c852378;	/* mr r5,r4 */
	memcpy(&dcp->dcp_bzero[0], (void *)bcopy_glue, sizeof(bcopy_glue));	
	memcpy(&dcp->dcp_bzero[8], (void *)memset, sizeof(dcp->dcp_bzero) - 8);
#endif

#ifdef __i386__
#error implement me
#endif

	memcpy(&dcp->dcp_memcpy[0], (void *)memcpy, sizeof(dcp->dcp_memcpy));

	/* dcp->dcp_bigcopy */
	return;
}
