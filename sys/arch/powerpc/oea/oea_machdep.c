/*	$NetBSD: oea_machdep.c,v 1.2 2003/02/05 07:05:20 matt Exp $	*/

/*
 * Copyright (C) 2002 Matt Thomas
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

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_multiprocessor.h"
#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/boot_flag.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif
 
#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

#include <powerpc/oea/bat.h>
#include <powerpc/oea/sr_601.h>
#include <powerpc/trap.h>
#include <powerpc/stdarg.h>
#include <powerpc/spr.h>
#include <powerpc/pte.h>
#include <powerpc/altivec.h>
#include <machine/powerpc.h>

char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

/*
 * Global variables used here and there
 */
extern struct user *proc0paddr;

struct bat battable[512];
register_t iosrtable[16];	/* I/O segments, for kernel_pmap setup */
paddr_t msgbuf_paddr;

void
oea_init(void (*handler)(void))
{
	extern int trapstart[], trapend[];
	extern int trapcode, trapsize;
	extern int sctrap, scsize;
	extern int alitrap, alisize;
	extern int dsitrap, dsisize;
	extern int dsi601trap, dsi601size;
	extern int decrint, decrsize;
	extern int tlbimiss, tlbimsize;
	extern int tlbdlmiss, tlbdlmsize;
	extern int tlbdsmiss, tlbdsmsize;
#if defined(DDB) || defined(KGDB)
	extern int ddblow, ddbsize;
#endif
#ifdef IPKDB
	extern int ipkdblow, ipkdbsize;
#endif
#ifdef ALTIVEC
	register_t msr;
#endif
	uintptr_t exc;
	register_t scratch;
	unsigned int cpuvers;
	size_t size;
	struct cpu_info * const ci = &cpu_info[0];

	mtspr(SPR_SPRG0, ci);
	cpuvers = mfpvr() >> 16;


	/*
	 * Initialize proc0 and current pcb and pmap pointers.
	 */
	KASSERT(ci != NULL);
	KASSERT(curcpu() == ci);
	lwp0.l_cpu = ci;
	KASSERT(lwp0.l_cpu != NULL);
	lwp0.l_addr = proc0paddr;
	memset(lwp0.l_addr, 0, sizeof *lwp0.l_addr);

	curpcb = &proc0paddr->u_pcb;
	curpm = curpcb->pcb_pmreal = curpcb->pcb_pm = pmap_kernel();

	/*
	 * Cause a PGM trap if we branch to 0.
	 */
	memset(0, 0, 0x100);

	/*
	 * Set up trap vectors.  Don't assume vectors are on 0x100.
	 */
	for (exc = EXC_RST; exc <= EXC_LAST; exc += 0x100) {
		switch (exc) {
		default:
			size = (size_t)&trapsize;
			memcpy((void *)exc, &trapcode, size);
			break;
#if 0
		case EXC_EXI:
			/*
			 * This one is (potentially) installed during autoconf
			 */
			break;
#endif
		case EXC_SC:
			size = (size_t)&scsize;
			memcpy((void *)EXC_SC, &sctrap, size);
			break;
		case EXC_ALI:
			size = (size_t)&alisize;
			memcpy((void *)EXC_ALI, &alitrap, size);
			break;
		case EXC_DSI:
			if (cpuvers == MPC601) {
				size = (size_t)&dsi601size;
				memcpy((void *)EXC_DSI, &dsi601trap, size);
			} else {
				size = (size_t)&dsisize;
				memcpy((void *)EXC_DSI, &dsitrap, size);
			}
			break;
		case EXC_DECR:
			size = (size_t)&decrsize;
			memcpy((void *)EXC_DECR, &decrint, size);
			break;
		case EXC_IMISS:
			size = (size_t)&tlbimsize;
			memcpy((void *)EXC_IMISS, &tlbimiss, size);
			break;
		case EXC_DLMISS:
			size = (size_t)&tlbdlmsize;
			memcpy((void *)EXC_DLMISS, &tlbdlmiss, size);
			break;
		case EXC_DSMISS:
			size = (size_t)&tlbdsmsize;
			memcpy((void *)EXC_DSMISS, &tlbdsmiss, size);
			break;
		case EXC_PERF:
			size = (size_t)&trapsize;
			memcpy((void *)EXC_PERF, &trapcode, size);
			memcpy((void *)EXC_VEC,  &trapcode, size);
			break;
#if defined(DDB) || defined(IPKDB) || defined(KGDB)
		case EXC_RUNMODETRC:
			if (cpuvers != MPC601) {
				size = (size_t)&trapsize;
				memcpy((void *)EXC_RUNMODETRC, &trapcode, size);
				break;
			}
			/* FALLTHROUGH */
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
#if defined(DDB) || defined(KGDB)
			size = (size_t)&ddbsize;
			memcpy((void *)exc, &ddblow, size);
#if defined(IPKDB)
#error "cannot enable IPKDB with DDB or KGDB"
#endif
#else
			size = (size_t)&ipkdbsize;
			memcpy((void *)exc, &ipkdblow, size);
#endif
			break;
#endif /* DDB || IPKDB || KGDB */
		}
#if 0
		exc += roundup(size, 32);
#endif
	}

	/*
	 * Get the cache sizes because install_extint calls __syncicache.
	 */
	cpu_probe_cache();

#define	MxSPR_MASK	0x7c1fffff
#define	MFSPR_MQ	0x7c0002a6
#define	MTSPR_MQ	0x7c0003a6
#define	NOP		0x60000000

#ifdef ALTIVEC
#define	MFSPR_VRSAVE	0x7c0042a6
#define	MTSPR_VRSAVE	0x7c0043a6
	
	/*
	 * Try to set the VEC bit in the MSR.  If it doesn't get set, we are
	 * not on a AltiVec capable processor.
	 */
	__asm __volatile (
	    "mfmsr %0; oris %1,%0,%2@h; mtmsr %1; isync; "
		"mfmsr %1; mtmsr %0; isync"
	    :	"=r"(msr), "=r"(scratch)
	    :	"J"(PSL_VEC));

	/*
	 * If we aren't on an AltiVec capable processor, we to need zap any of
	 * sequences we save/restore the VRSAVE SPR into NOPs.
	 */
	if (scratch & PSL_VEC) {
		cpu_altivec = 1;
	} else {
		int *ip = trapstart;
		
		for (; ip < trapend; ip++) {
			if ((ip[0] & MxSPR_MASK) == MFSPR_VRSAVE) {
				ip[0] = NOP;	/* mfspr */
				ip[1] = NOP;	/* stw */
			} else if ((ip[0] & MxSPR_MASK) == MTSPR_VRSAVE) {
				ip[-1] = NOP;	/* lwz */
				ip[0] = NOP;	/* mtspr */
			}
		}
	}
#endif

	/*
	 * If we aren't on a MPC601 processor, we to need zap any of
	 * sequences we save/restore the MQ SPR into NOPs.
	 */
	if (cpuvers != MPC601) {
		int *ip = trapstart;
		
		for (; ip < trapend; ip++) {
			if ((ip[0] & MxSPR_MASK) == MFSPR_MQ) {
				ip[0] = NOP;	/* mfspr */
				ip[1] = NOP;	/* stw */
			} else if ((ip[0] & MxSPR_MASK) == MTSPR_MQ) {
				ip[-1] = NOP;	/* lwz */
				ip[0] = NOP;	/* mtspr */
			}
		}
	}

	if (!cpu_altivec || cpuvers != MPC601) {
		/*
		 * Sync the changed instructions.
		 */
		__syncicache((void *) trapstart,
		    (uintptr_t) trapend - (uintptr_t) trapstart);
	}

	/*
	 * external interrupt handler install
	 */
	if (handler)
		oea_install_extint(handler);

	__syncicache(0, EXC_LAST + 0x100);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
	__asm __volatile ("sync; mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
	    : "=r"(scratch)
	    : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));

	KASSERT(curcpu() == ci);
}

void
mpc601_ioseg_add(paddr_t pa, register_t len)
{
	const u_int i = pa >> ADDR_SR_SHFT;

	if (len != BAT_BL_256M)
		panic("mpc601_ioseg_add: len != 256M");

	/*
	 * Translate into an I/O segment, load it, and stash away for use
	 * in pmap_bootstrap().
	 */
	iosrtable[i] = SR601(SR601_Ks, SR601_BUID_MEMFORCED, 0, i);
	__asm __volatile ("mtsrin %0,%1"
	    ::	"r"(iosrtable[i]),
		"r"(pa));
}

void
oea_iobat_add(paddr_t pa, register_t len)
{
	static int n = 1;
	const u_int i = pa >> 28;
	battable[i].batl = BATL(pa, BAT_I|BAT_G, BAT_PP_RW);
	battable[i].batu = BATU(pa, len, BAT_Vs);

	/*
	 * Let's start loading the BAT registers.
	 */
	switch (n) {
	case 1:
		__asm __volatile ("mtdbatl 1,%0; mtdbatu 1,%1;"
		    ::	"r"(battable[i].batl),
			"r"(battable[i].batu));
		n = 2;
		break;
	case 2:
		__asm __volatile ("mtdbatl 2,%0; mtdbatu 2,%1;"
		    ::	"r"(battable[i].batl),
			"r"(battable[i].batu));
		n = 3;
		break;
	case 3:
		__asm __volatile ("mtdbatl 3,%0; mtdbatu 3,%1;"
		    ::	"r"(battable[i].batl),
			"r"(battable[i].batu));
		n = 4;
		break;
	default:
		break;
	}
}

void
oea_batinit(paddr_t pa, ...)
{
	struct mem_region *allmem, *availmem, *mp;
	int i;
	unsigned int cpuvers;
	va_list ap;

	cpuvers = mfpvr() >> 16;

	/*
	 * Initialize BAT registers to unmapped to not generate
	 * overlapping mappings below.
	 *
	 * The 601's implementation differs in the Valid bit being situated
	 * in the lower BAT register, and in being a unified BAT only whose
	 * four entries are accessed through the IBAT[0-3] SPRs.
	 *
	 * Also, while the 601 does distinguish between supervisor/user
	 * protection keys, it does _not_ distinguish distinguish between  
	 * validity in supervisor/user mode.
	 */
	if (cpuvers == MPC601) {
		__asm __volatile ("mtibatl 0,%0" :: "r"(0));
		__asm __volatile ("mtibatl 1,%0" :: "r"(0));
		__asm __volatile ("mtibatl 2,%0" :: "r"(0));
		__asm __volatile ("mtibatl 3,%0" :: "r"(0));
	} else {
		__asm __volatile ("mtibatu 0,%0" :: "r"(0));
		__asm __volatile ("mtibatu 1,%0" :: "r"(0));
		__asm __volatile ("mtibatu 2,%0" :: "r"(0));
		__asm __volatile ("mtibatu 3,%0" :: "r"(0));
		__asm __volatile ("mtdbatu 0,%0" :: "r"(0));
		__asm __volatile ("mtdbatu 1,%0" :: "r"(0));
		__asm __volatile ("mtdbatu 2,%0" :: "r"(0));
		__asm __volatile ("mtdbatu 3,%0" :: "r"(0));
	}

	/*
	 * Set up BAT to map physical memory
	 */
	if (cpuvers == MPC601) {
		/*
		 * Set up battable to map the lowest 256 MB area.
		 * Map the lowest 32 MB area via BAT[0-3];
		 * BAT[01] are fixed, BAT[23] are floating.
		 */
		for (i = 0; i < 32; i++) {
			battable[i].batl = BATL601(i << 23,
			   BAT601_BSM_8M, BAT601_V);
			battable[i].batu = BATU601(i << 23,
			    BAT601_M, BAT601_Ku, BAT601_PP_NONE);
		}
		__asm __volatile ("mtibatu 0,%1; mtibatl 0,%0"
		    :: "r"(battable[0x00000000 >> 23].batl),
		       "r"(battable[0x00000000 >> 23].batu));
		__asm __volatile ("mtibatu 1,%1; mtibatl 1,%0"
		    :: "r"(battable[0x00800000 >> 23].batl),
		       "r"(battable[0x00800000 >> 23].batu));
		__asm __volatile ("mtibatu 2,%1; mtibatl 2,%0"
		    :: "r"(battable[0x01000000 >> 23].batl),
		       "r"(battable[0x01000000 >> 23].batu));
		__asm __volatile ("mtibatu 3,%1; mtibatl 3,%0"
		    :: "r"(battable[0x01800000 >> 23].batl),
		       "r"(battable[0x01800000 >> 23].batu));
	} else {
		/*
		 * Set up BAT0 to only map the lowest 256 MB area
		 */
		battable[0].batl = BATL(0x00000000, BAT_M, BAT_PP_RW);
		battable[0].batu = BATU(0x00000000, BAT_BL_256M, BAT_Vs);

		__asm __volatile ("mtibatl 0,%0; mtibatu 0,%1;"
				  "mtdbatl 0,%0; mtdbatu 0,%1;"
		    ::	"r"(battable[0].batl), "r"(battable[0].batu));
	}

	/*
	 * Now setup other fixed bat registers
	 *
	 * Note that we still run in real mode, and the BAT
	 * registers were cleared above.
	 */

	va_start(ap, pa);

	/*
	 * Add any I/O BATs specificed;
	 * use I/O segments on the BAT-starved 601.
	 */
	if (cpuvers == MPC601) {
		while (pa != 0) {
			register_t len = va_arg(ap, register_t);
			mpc601_ioseg_add(pa, len);
			pa = va_arg(ap, paddr_t);
		}
	} else {
		while (pa != 0) {
			register_t len = va_arg(ap, register_t);
			oea_iobat_add(pa, len);
			pa = va_arg(ap, paddr_t);
		}
	}

	va_end(ap);

	/*
	 * Set up battable to map all RAM regions.
	 * This is here because mem_regions() call needs bat0 set up.
	 */
	mem_regions(&allmem, &availmem);
	if (cpuvers == MPC601) {
		for (mp = allmem; mp->size; mp++) {
			paddr_t pa = mp->start & 0xff800000;
			paddr_t end = mp->start + mp->size;

			do {
				u_int i = pa >> 23;

				battable[i].batl =
				    BATL601(pa, BAT601_BSM_8M, BAT601_V);
				battable[i].batu =
				    BATU601(pa, BAT601_M, BAT601_Ku, BAT601_PP_NONE);
				pa += (1 << 23);
			} while (pa < end);
		}
	} else {
		for (mp = allmem; mp->size; mp++) {
			paddr_t pa = mp->start & 0xf0000000;
			paddr_t end = mp->start + mp->size;

			do {
				u_int i = pa >> 28;

				battable[i].batl =
				    BATL(pa, BAT_M, BAT_PP_RW);
				battable[i].batu =
				    BATU(pa, BAT_BL_256M, BAT_Vs);
				pa += SEGMENT_LENGTH;
			} while (pa < end);
		}
	}
}

void
oea_install_extint(void (*handler)(void))
{
	extern int extint, extsize;
	extern int extint_call;
	uintptr_t offset = (uintptr_t)handler - (uintptr_t)&extint_call;
	int omsr, msr;

#ifdef	DIAGNOSTIC
	if (offset > 0x1ffffff)
		panic("install_extint: %p too far away (%#lx)", handler,
		    (unsigned long) offset);
#endif
	__asm __volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
	    :	"=r" (omsr), "=r" (msr)
	    :	"K" ((u_short)~PSL_EE));
	extint_call = (extint_call & 0xfc000003) | offset;
	memcpy((void *)EXC_EXI, &extint, (size_t)&extsize);
	__syncicache((void *)&extint_call, sizeof extint_call);
	__syncicache((void *)EXC_EXI, (int)&extsize);
	__asm __volatile ("mtmsr %0" :: "r"(omsr));
}

/*
 * Machine dependent startup code.
 */
void
oea_startup(const char *model)
{
	uintptr_t sz;
	u_int i;
	u_long base, residual;
	int error;
	caddr_t v;
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	KASSERT(curcpu() != NULL);
	KASSERT(lwp0.l_cpu != NULL);

	/*
	 * If the msgbuf is not in segment 0, allocate KVA for it and access
	 * it via mapped pages.  [This prevents unneeded BAT switches.]
	 */
        sz = round_page(MSGBUFSIZE);
	v = (caddr_t) msgbuf_paddr;
	if (msgbuf_paddr + sz > SEGMENT_LENGTH) {
		minaddr = 0;
		if (uvm_map(kernel_map, &minaddr, sz,
				NULL, UVM_UNKNOWN_OFFSET, 0,
				UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE,
				    UVM_INH_NONE, UVM_ADV_NORMAL, 0)) != 0)
			panic("startup: cannot allocate VM for msgbuf");
		v = (caddr_t)minaddr;
		for (i = 0; i < sz; i += NBPG) {
			pmap_kenter_pa(minaddr + i, msgbuf_paddr + i,
			    VM_PROT_READ|VM_PROT_WRITE);
		}
		pmap_update(pmap_kernel());
	}
	initmsgbuf(v, sz);

	printf("%s", version);
	if (model != NULL)
		printf("Model: %s\n", model);
	cpu_identify(NULL, 0);

	format_bytes(pbuf, sizeof(pbuf), ctob((u_int)physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (uintptr_t)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v, NULL) - v != sz)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 * Allocate the buffer starting at the top of the kernel VM space.
	 */
	sz = MAXBSIZE * nbuf;
	minaddr = VM_MAX_KERNEL_ADDRESS - round_page(sz);
	if (uvm_map(kernel_map, &minaddr, round_page(sz),
		NULL, UVM_UNKNOWN_OFFSET, 0,
		UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
			    UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	buffers = (char *)minaddr;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	if (base >= MAXBSIZE) {
		/* Don't want to alloc more physical mem than ever needed */
		base = MAXBSIZE;
		residual = 0;
	}
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		curbuf = (vaddr_t)buffers + i * MAXBSIZE;
		curbufsize = NBPG * (i < residual ? base + 1 : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	pmap_update(pmap_kernel());

	/*
	 * Allocate away the pages that map to 0xDEA[CDE]xxxx.  Do this after
	 * the bufpages are allocated in case they overlap since it's not
	 * fatal if we can't allocate these.
	 */
	minaddr = 0xDEAC0000;
	error = uvm_map(kernel_map, &minaddr, 0x30000,
	    NULL, UVM_UNKNOWN_OFFSET, 0,
	    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,  
			UVM_ADV_NORMAL, UVM_FLAG_FIXED));
	if (error != 0 || minaddr != 0xDEAC0000) {
		printf("oea_startup: failed to allocate DEAD "
		    "ZONE: error=%d\n", error);
		minaddr = 0;
	}
 
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time. These
	 * submaps will be allocated after the dead zone.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, FALSE, NULL);

#ifndef PMAP_MAP_POOLPAGE
	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    mclbytes*nmbclusters, VM_MAP_INTRSAFE, FALSE, NULL);
#endif

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %u buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up the buffers.
	 */
	bufinit();

#ifdef ALTIVEC
	if (cpu_altivec)
		init_vec();
#endif
}

/*
 * Crash dump handling.
 */

void
oea_dumpsys(void)
{
	printf("dumpsys: TBD\n");
}

/*
 * Soft networking interrupts.
 */
void
softnet(int pendisr)
{
#define DONETISR(bit, fn) do {		\
	if (pendisr & (1 << bit))	\
		(*fn)();		\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR

}

/*
 * Convert kernel VA to physical address
 */
paddr_t
kvtop(caddr_t addr)
{
	vaddr_t va;
	paddr_t pa;
	uintptr_t off;
	extern char end[];

	if (addr < end)
		return (paddr_t)addr;

	va = trunc_page((vaddr_t)addr);
	off = (uintptr_t)addr - va;

	if (pmap_extract(pmap_kernel(), va, &pa) == FALSE) {
		/*printf("kvtop: zero page frame (va=0x%x)\n", addr);*/
		return (paddr_t)addr;
	}

	return(pa + off);
}

/*
 * Allocate vm space and mapin the I/O address
 */
void *
mapiodev(paddr_t pa, psize_t len)
{
	paddr_t faddr;
	vaddr_t taddr, va;
	int off;

	faddr = trunc_page(pa);
	off = pa - faddr;
	len = round_page(off + len);
	va = taddr = uvm_km_valloc(kernel_map, len);

	if (va == 0)
		return NULL;

	for (; len > 0; len -= NBPG) {
		pmap_kenter_pa(taddr, faddr, VM_PROT_READ | VM_PROT_WRITE);
		faddr += NBPG;
		taddr += NBPG;
	}
	pmap_update(pmap_kernel());
	return (void *)(va + off);
}
