/*	$NetBSD: ibm4xx_machdep.c,v 1.18.6.1 2017/12/03 11:36:36 jdolecek Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: ibm4xx_machdep.c,v 1.18.6.1 2017/12/03 11:36:36 jdolecek Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_modular.h"
#include "ksyms.h" /* for NKSYMS */

#include <sys/param.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/ksyms.h>

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
#include <powerpc/pcb.h>
#include <machine/trap.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <powerpc/ibm4xx/cpu.h>

/*
 * Global variables used here and there
 */
paddr_t msgbuf_paddr;
vaddr_t msgbuf_vaddr;
char msgbuf[MSGBUFSIZE];

#if NKSYMS || defined(DDB) || defined(MODULAR)
void *startsym, *endsym;
#endif

/*
 * Trap vectors
 */
extern const uint32_t defaulttrap[], defaultsize;
extern const uint32_t sctrap[], scsize;
extern const uint32_t accesstrap[], accesssize;
extern const uint32_t criticaltrap[], criticalsize;
extern const uint32_t tlbimiss4xx[], tlbim4size;
extern const uint32_t tlbdmiss4xx[], tlbdm4size;
extern const uint32_t pitfitwdog[], pitfitwdogsize;
extern const uint32_t errata51handler[], errata51size;
#if defined(DDB)
extern const uint32_t ddblow[], ddbsize;
#elif defined(IPKDB)
extern const uint32_t ipkdblow[], ipkdbsize;
#endif
static const struct exc_info trap_table[] = {
	{ EXC_SC,	sctrap,		(uintptr_t)&scsize },
	{ EXC_ALI,	accesstrap,	(uintptr_t)&accesssize },
	{ EXC_DSI,	accesstrap,	(uintptr_t)&accesssize },
	{ EXC_MCHK,	criticaltrap,	(uintptr_t)&criticalsize },
	{ EXC_ITMISS,	tlbimiss4xx,	(uintptr_t)&tlbim4size },
	{ EXC_DTMISS,	tlbdmiss4xx,	(uintptr_t)&tlbdm4size },
	{ EXC_PIT,	pitfitwdog,	(uintptr_t)&pitfitwdogsize },
	{ EXC_DEBUG,	criticaltrap,	(uintptr_t)&criticalsize },
	{ (EXC_DTMISS|EXC_ALI),
			errata51handler, (uintptr_t)&errata51size },
#if defined(DDB)
	{ EXC_PGM,	ddblow,		(uintptr_t)&ddbsize },
#elif defined(IPKDB)
	{ EXC_PGM,	ipkdblow,	(uintptr_t)&ipkdbsize },
#endif
};

/*
 * Install a trap vector. We cannot use memcpy because the
 * destination may be zero.
 */
static void
trap_copy(const uint32_t *src, vaddr_t dest, size_t len)
{
	uint32_t *dest_p = (void *)dest;

	while (len > 0) {
		*dest_p++ = *src++;
		len -= sizeof(uint32_t);
	}
}

/*
 * ibm4xx_init:
 */
void
ibm4xx_init(vaddr_t startkernel, vaddr_t endkernel, void (*handler)(void))
{
	/* Initialize cache info for memcpy, etc. */
	cpu_probe_cache();

	/*
	 * Initialize current pcb and pmap pointers.
	 */
	KASSERT(curcpu() == &cpu_info[0]);
	KASSERT(lwp0.l_cpu == curcpu());
	KASSERT(curlwp == &lwp0);

	curpcb = lwp_getpcb(curlwp);
	memset(curpcb, 0, sizeof(struct pcb));

	curpcb->pcb_pm = pmap_kernel();

	for (uintptr_t exc = EXC_RSVD; exc <= EXC_LAST; exc += 0x100) {
		trap_copy(defaulttrap, exc, (uintptr_t)&defaultsize);
	}

	for (size_t i = 0; i < __arraycount(trap_table); i++) {
		KASSERT(trap_table[i].exc_size <= 0x100);
		trap_copy(trap_table[i].exc_addr, trap_table[i].exc_vector,
		    trap_table[i].exc_size);
	}

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);

	mtspr(SPR_EVPR, 0);		/* Set Exception vector base */

	/* Handle trap instruction as PGM exception */
	mtspr(SPR_DBCR0, mfspr(SPR_DBCR0) & ~DBCR0_TDE);

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

	/*
	 * turn on console after enable translation
	 */
	consinit();

	uvm_md_init();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

	/*
	 * Let's take all the indirect calls via our stubs and patch
	 * them to be direct calls.
	 */
	cpu_fixup_stubs();

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((uintptr_t)endsym - (uintptr_t)startsym,
	    startsym, endsym);
#endif
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
	KASSERT(curcpu()->ci_idepth == -1);

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
		    msgbuf_paddr + i * PAGE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, 0);
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
