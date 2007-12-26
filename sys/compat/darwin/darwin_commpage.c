/*	$NetBSD: darwin_commpage.c,v 1.12.4.1 2007/12/26 19:48:47 ad Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: darwin_commpage.c,v 1.12.4.1 2007/12/26 19:48:47 ad Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/lwp.h>
#include <sys/proc.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <sys/cpu.h>
#include <machine/darwin_machdep.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>
#include <uvm/uvm.h>

#include <compat/darwin/darwin_commpage.h>

/* XXX: this does not belong here! */
#ifdef __powerpc__
#include "opt_altivec.h"
#endif

#ifdef DEBUG_DARWIN
#define DPRINTF(a) uprintf a
#else
#define DPRINTF(a) 
#endif

static struct uvm_object *darwin_commpage_uao = NULL;

static void darwin_commpage_init(struct darwin_commpage *);

int
darwin_commpage_map(struct proc *p)
{
	int error;
	vaddr_t kvaddr;
	vaddr_t pvaddr;
	size_t memsize;

	/*
	 * XXX This crashes. For now we map only one page,
	 * to avoid the crash, but this ought to be fixed
	 */
	memsize = round_page(sizeof(struct darwin_commpage));

	if (darwin_commpage_uao == NULL) {
		darwin_commpage_uao = uao_create(memsize, 0);
		kvaddr = vm_map_min(kernel_map);

		error = uvm_map(kernel_map, &kvaddr, memsize,
		    darwin_commpage_uao, 0, PAGE_SIZE,
		    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
		    UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
		if (error != 0) {
			DPRINTF(("kernel uvm_map darwin_compage failed "
			    "(error %d)\n", error));
			uao_detach(darwin_commpage_uao);
			darwin_commpage_uao = NULL;
			return -1;
		}

		error = uvm_map_pageable(kernel_map, kvaddr,
		    kvaddr + memsize, FALSE, 0);
		if (error != 0) {
			DPRINTF(("kernel uvm_map_pageable darwin_compage "
			    "failed (error %d)\n", error));
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
	    UVM_MAPFLAG(UVM_PROT_RX, UVM_PROT_RX,
	    UVM_INH_SHARE, UVM_ADV_NORMAL, UVM_FLAG_FIXED))) != 0) {
		DPRINTF(("user uvm_map darwin_commpage failed at "
		    "0x%08lx/memsize (error %d)\n", (long)pvaddr, error));
		return -1;
	}

	DPRINTF(("mapped darwin_commpage at 0x%08lx\n", (long)pvaddr));

	return 0;
}

#define DCP_MEMCPY(x) {							\
	size_t len;							\
									\
	len = (size_t)darwin_commpage_##x##_size;			\
									\
	if (len > sizeof(dcp->dcp_##x)) {				\
		printf("darwin_commpage: %s too big (%d/%d)\n", #x,	\
			len, sizeof(dcp->dcp_##x));			\
	} else {							\
		memcpy(dcp->dcp_##x, (void *)darwin_commpage_##x, len);	\
	}								\
}

void
darwin_commpage_init(struct darwin_commpage *dcp)
{
	/*
	 * XXX Only one page is mapped yet (see higher in the file)
	 */
	(void)memset(dcp, 0, sizeof(*dcp));

	dcp->dcp_version = DARWIN_COMMPAGE_VERSION;
	dcp->dcp_ncpu = ncpu;
	dcp->dcp_cap |= (ncpu << DARWIN_CAP_NCPUSHIFT);

#ifdef DARWIN_CAP_UP
	if (ncpu == 1)
		dcp->dcp_cap |= DARWIN_CAP_UP;
#endif

	/* XXX: This needs to be processor specific */
#ifdef ALTIVEC
	dcp->dcp_vector = 1;
	dcp->dcp_cap |= DARWIN_CAP_ALTIVEC;
#endif

#if defined(_LP64) && defined(DARWIN_CAP_64BIT)
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

	dcp->dcp_2pow52 = 4503599627370496ULL;	/* 2^52 */
	dcp->dcp_10pow6 = 1000000ULL;		/* 10^6 */

	/*
	 * On Darwin, these are maintained up to date by the kernel
	 */
	dcp->dcp_timebase = 0; /* XXX */
	dcp->dcp_timestamp = 0; /* XXX */
	dcp->dcp_secpertick = hz; /* XXX Not sure */

	DCP_MEMCPY(mach_absolute_time);
	DCP_MEMCPY(spinlock_try);
	DCP_MEMCPY(spinlock_lock);
	DCP_MEMCPY(spinlock_unlock);
	DCP_MEMCPY(pthread_getspecific);
	DCP_MEMCPY(gettimeofday);
	DCP_MEMCPY(sys_dcache_flush);
	DCP_MEMCPY(sys_icache_invalidate);
	DCP_MEMCPY(pthread_self);
	DCP_MEMCPY(spinlock_relinquish);
	DCP_MEMCPY(bzero);
	DCP_MEMCPY(bcopy);
	DCP_MEMCPY(memcpy);
	DCP_MEMCPY(bigcopy);
}
