/*	$NetBSD: ibm4xx_machdep.c,v 1.5.26.2 2007/03/12 05:49:52 rmind Exp $	*/
/*	Original: ibm40x_machdep.c,v 1.3 2005/01/17 17:19:36 shige Exp $ */

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibm4xx_machdep.c,v 1.5.26.2 2007/03/12 05:49:52 rmind Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#if defined(KGDB)
#include <sys/kgdb.h>
#endif

#if defined(IPKDB)
#include <ipkdb/ipkdb.h>
#endif

#include <machine/powerpc.h>
#include <powerpc/spr.h>
#include <machine/trap.h>

/*
 * Global variables used here and there
 */
extern struct user *proc0paddr;
paddr_t msgbuf_paddr;
vaddr_t msgbuf_vaddr;
char msgbuf[MSGBUFSIZE];

/*
 * ibm4xx_init:
 */
void
ibm4xx_init(void (*handler)(void))
{
	extern int defaulttrap, defaultsize;
	extern int sctrap, scsize;
	extern int alitrap, alisize;
	extern int dsitrap, dsisize;
	extern int isitrap, isisize;
	extern int mchktrap, mchksize;
	extern int tlbimiss4xx, tlbim4size;
	extern int tlbdmiss4xx, tlbdm4size;
	extern int pitfitwdog, pitfitwdogsize;
	extern int debugtrap, debugsize;
	extern int errata51handler, errata51size;
#ifdef DDB
	extern int ddblow, ddbsize;
#endif
#ifdef IPKDB
	extern int ipkdblow, ipkdbsize;
#endif
	uintptr_t exc;
	struct cpu_info * const ci = curcpu();

	/* Initialize cache info for memcpy, etc. */
	cpu_probe_cache();

	/*
	 * Initialize lwp0 and current pcb and pmap pointers.
	 */
	KASSERT(ci != NULL);
	KASSERT(curcpu() == ci);
	lwp0.l_cpu = ci;
	lwp0.l_addr = proc0paddr;
	memset(lwp0.l_addr, 0, sizeof *lwp0.l_addr);
	KASSERT(lwp0.l_cpu != NULL);

	curpcb = &proc0paddr->u_pcb;
        memset(curpcb, 0, sizeof(*curpcb));
	curpcb->pcb_pm = pmap_kernel();

	/*
	 * Set up trap vectors
	 */
	for (exc = EXC_RSVD; exc <= EXC_LAST; exc += 0x100)
		switch (exc) {
		default:
			memcpy((void *)exc, &defaulttrap, (size_t)&defaultsize);
			break;
		case EXC_EXI:
			/*
			 * This one is (potentially) installed during autoconf
			 */
			break;
		case EXC_SC:
			memcpy((void *)EXC_SC, &sctrap, (size_t)&scsize);
			break;
		case EXC_ALI:
			memcpy((void *)EXC_ALI, &alitrap, (size_t)&alisize);
			break;
		case EXC_DSI:
			memcpy((void *)EXC_DSI, &dsitrap, (size_t)&dsisize);
			break;
		case EXC_ISI:
			memcpy((void *)EXC_ISI, &isitrap, (size_t)&isisize);
			break;
		case EXC_MCHK:
			memcpy((void *)EXC_MCHK, &mchktrap, (size_t)&mchksize);
			break;
		case EXC_ITMISS:
			memcpy((void *)EXC_ITMISS, &tlbimiss4xx,
				(size_t)&tlbim4size);
			break;
		case EXC_DTMISS:
			memcpy((void *)EXC_DTMISS, &tlbdmiss4xx,
				(size_t)&tlbdm4size);
			break;
		/* 
		 * EXC_PIT, EXC_FIT, EXC_WDOG handlers 
		 * are spaced by 0x10 bytes only.. 
		 */
		case EXC_PIT:	
			memcpy((void *)EXC_PIT, &pitfitwdog,
				(size_t)&pitfitwdogsize);
			break;
		case EXC_DEBUG:
			memcpy((void *)EXC_DEBUG, &debugtrap,
				(size_t)&debugsize);
			break;
		case EXC_DTMISS|EXC_ALI:
                        /* PPC405GP Rev D errata item 51 */	
			memcpy((void *)(EXC_DTMISS|EXC_ALI), &errata51handler,
				(size_t)&errata51size);
			break;
#if defined(DDB) || defined(IPKDB)
		case EXC_PGM:
#if defined(DDB)
			memcpy((void *)exc, &ddblow, (size_t)&ddbsize);
#elif defined(IPKDB)
			memcpy((void *)exc, &ipkdblow, (size_t)&ipkdbsize);
#endif
#endif /* DDB | IPKDB */
			break;
		}

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);
	mtspr(SPR_EVPR, 0);		/* Set Exception vector base */

	consinit();

	/* Handle trap instruction as PGM exception */
	{
	  int dbcr0;
	  __asm volatile("mfspr %0,%1":"=r"(dbcr0):"K"(SPR_DBCR0));
	  __asm volatile("mtspr %0,%1"::"K"(SPR_DBCR0),"r"(dbcr0 & ~DBCR0_TDE));
	}

	/*
	 * external interrupt handler install
	 */
        if (handler)
	    ibm4xx_install_extint(handler);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
	__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
		      : : "r"(0), "K"(PSL_IR|PSL_DR)); 
	/* XXXX PSL_ME - With ME set kernel gets stuck... */

	KASSERT(curcpu() == ci);
}

void
ibm4xx_install_extint(void (*handler)(void))
{
	extern int extint, extsize;
	extern u_long extint_call;
	u_long offset = (u_long)handler - (u_long)&extint_call;
	int msr;

#ifdef	DIAGNOSTIC
	if (offset > 0x1ffffff)
		panic("install_extint: too far away");
#endif
	__asm volatile ("mfmsr %0; wrteei 0" : "=r"(msr));
	extint_call = (extint_call & 0xfc000003) | offset;
	memcpy((void *)EXC_EXI, &extint, (size_t)&extsize);
	__syncicache((void *)&extint_call, sizeof extint_call);
	__syncicache((void *)EXC_EXI, (int)&extsize);
	__asm volatile ("mtmsr %0" :: "r"(msr));
}

/*
 * ibm4xx_cpu_startup:
 * Machine dependent startup code.
 */
void
ibm4xx_cpu_startup(const char *model)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	KASSERT(curcpu() != NULL);
	KASSERT(lwp0.l_cpu != NULL);
	KASSERT(curcpu()->ci_intstk != 0);
	KASSERT(curcpu()->ci_intrdepth == -1);

	/*
	 * Initialize error message buffer (at end of core).
	 */
#if 0	/* For some reason this fails... --Artem
	 * Besides, do we really have to put it at the end of core?
	 * Let's use static buffer for now
	 */
	if (!(msgbuf_vaddr = uvm_km_alloc(kernel_map, round_page(MSGBUFSIZE),
	    0, UVM_KMF_VAONLY)))
		panic("startup: no room for message buffer");
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa(msgbuf_vaddr + i * PAGE_SIZE,
		    msgbuf_paddr + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE);
	initmsgbuf((void *)msgbuf_vaddr, round_page(MSGBUFSIZE));
#else
	initmsgbuf((void *)msgbuf, round_page(MSGBUFSIZE));
#endif

	printf("%s%s", copyright, version);
	if (model != NULL)
		printf("Model: %s\n", model);

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, VM_MAP_PAGEABLE, false, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

/*
 * ibm4xx_dumpsys:
 * Crash dump handling.
 */
void
ibm4xx_dumpsys(void)
{

	printf("dumpsys: TBD\n");
}
