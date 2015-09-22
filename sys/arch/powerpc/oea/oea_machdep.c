/*	$NetBSD: oea_machdep.c,v 1.71.6.1 2015/09/22 12:05:50 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: oea_machdep.c,v 1.71.6.1 2015/09/22 12:05:50 skrll Exp $");

#include "opt_ppcarch.h"
#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_multiprocessor.h"
#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#ifdef DDB
#include <powerpc/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif
 
#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

#include <machine/powerpc.h>

#include <powerpc/trap.h>
#include <powerpc/spr.h>
#include <powerpc/pte.h>
#include <powerpc/altivec.h>
#include <powerpc/pcb.h>

#include <powerpc/oea/bat.h>
#include <powerpc/oea/cpufeat.h>
#include <powerpc/oea/spr.h>
#include <powerpc/oea/sr_601.h>

char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

struct vm_map *phys_map = NULL;

/*
 * Global variables used here and there
 */
static void trap0(void *);

/* XXXSL: The battable is not initialized to non-zero for PPC_OEA64 and PPC_OEA64_BRIDGE */
struct bat battable[BAT_VA2IDX(0xffffffff)+1];

register_t iosrtable[16];	/* I/O segments, for kernel_pmap setup */
#ifndef MSGBUFADDR
paddr_t msgbuf_paddr;
#endif

extern int dsitrap_fix_dbat4[];
extern int dsitrap_fix_dbat5[];
extern int dsitrap_fix_dbat6[];
extern int dsitrap_fix_dbat7[];

void
oea_init(void (*handler)(void))
{
	extern int trapcode[], trapsize[];
	extern int sctrap[], scsize[];
	extern int alitrap[], alisize[];
	extern int dsitrap[], dsisize[];
	extern int trapstart[], trapend[];
#ifdef PPC_OEA601
	extern int dsi601trap[], dsi601size[];
#endif
	extern int decrint[], decrsize[];
	extern int tlbimiss[], tlbimsize[];
	extern int tlbdlmiss[], tlbdlmsize[];
	extern int tlbdsmiss[], tlbdsmsize[];
#if defined(DDB) || defined(KGDB)
	extern int ddblow[], ddbsize[];
#endif
#ifdef IPKDB
	extern int ipkdblow[], ipkdbsize[];
#endif
#ifdef ALTIVEC
	register_t msr;
#endif
	uintptr_t exc, exc_base;
#if defined(ALTIVEC) || defined(PPC_OEA)
	register_t scratch;
#endif
	unsigned int cpuvers;
	size_t size;
	struct cpu_info * const ci = &cpu_info[0];

#ifdef PPC_HIGH_VEC
	exc_base = EXC_HIGHVEC;
#else
	exc_base = 0;
#endif
	KASSERT(mfspr(SPR_SPRG0) == (uintptr_t)ci);

#if defined (PPC_OEA64_BRIDGE) && defined (PPC_OEA)
	if (oeacpufeat & OEACPU_64_BRIDGE)
		pmap_setup64bridge();
	else
		pmap_setup32();
#endif


	cpuvers = mfpvr() >> 16;

	/*
	 * Initialize proc0 and current pcb and pmap pointers.
	 */
	(void) ci;
	KASSERT(ci != NULL);
	KASSERT(curcpu() == ci);
	KASSERT(lwp0.l_cpu == ci);

	curpcb = lwp_getpcb(&lwp0);
	memset(curpcb, 0, sizeof(struct pcb));

#ifdef ALTIVEC
	/*
	 * Initialize the vectors with NaNs
	 */
	for (scratch = 0; scratch < 32; scratch++) {
		curpcb->pcb_vr.vreg[scratch][0] = 0x7FFFDEAD;
		curpcb->pcb_vr.vreg[scratch][1] = 0x7FFFDEAD;
		curpcb->pcb_vr.vreg[scratch][2] = 0x7FFFDEAD;
		curpcb->pcb_vr.vreg[scratch][3] = 0x7FFFDEAD;
	}
#endif
	curpm = curpcb->pcb_pm = pmap_kernel();

	/*
	 * Cause a PGM trap if we branch to 0.
	 *
	 * XXX GCC4.1 complains about memset on address zero, so
	 * don't use the builtin.
	 */
#undef memset
	memset(0, 0, 0x100);

	/*
	 * Set up trap vectors.  Don't assume vectors are on 0x100.
	 */
	for (exc = exc_base; exc <= exc_base + EXC_LAST; exc += 0x100) {
		switch (exc - exc_base) {
		default:
			size = (size_t)trapsize;
			memcpy((void *)exc, trapcode, size);
			break;
#if 0
		case EXC_EXI:
			/*
			 * This one is (potentially) installed during autoconf
			 */
			break;
#endif
		case EXC_SC:
			size = (size_t)scsize;
			memcpy((void *)exc, sctrap, size);
			break;
		case EXC_ALI:
			size = (size_t)alisize;
			memcpy((void *)exc, alitrap, size);
			break;
		case EXC_DSI:
#ifdef PPC_OEA601
			if (cpuvers == MPC601) {
				size = (size_t)dsi601size;
				memcpy((void *)exc, dsi601trap, size);
				break;
			} else
#endif /* PPC_OEA601 */
			if (oeacpufeat & OEACPU_NOBAT) {
				size = (size_t)alisize;
				memcpy((void *)exc, alitrap, size);
			} else {
				size = (size_t)dsisize;
				memcpy((void *)exc, dsitrap, size);
			}
			break;
		case EXC_DECR:
			size = (size_t)decrsize;
			memcpy((void *)exc, decrint, size);
			break;
		case EXC_IMISS:
			size = (size_t)tlbimsize;
			memcpy((void *)exc, tlbimiss, size);
			break;
		case EXC_DLMISS:
			size = (size_t)tlbdlmsize;
			memcpy((void *)exc, tlbdlmiss, size);
			break;
		case EXC_DSMISS:
			size = (size_t)tlbdsmsize;
			memcpy((void *)exc, tlbdsmiss, size);
			break;
		case EXC_PERF:
			size = (size_t)trapsize;
			memcpy((void *)exc, trapcode, size);
			memcpy((void *)(exc_base + EXC_VEC),  trapcode, size);
			break;
#if defined(DDB) || defined(IPKDB) || defined(KGDB)
		case EXC_RUNMODETRC:
#ifdef PPC_OEA601
			if (cpuvers != MPC601) {
#endif
				size = (size_t)trapsize;
				memcpy((void *)exc, trapcode, size);
				break;
#ifdef PPC_OEA601
			}
			/* FALLTHROUGH */
#endif
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
#if defined(DDB) || defined(KGDB)
			size = (size_t)ddbsize;
			memcpy((void *)exc, ddblow, size);
#if defined(IPKDB)
#error "cannot enable IPKDB with DDB or KGDB"
#endif
#else
			size = (size_t)ipkdbsize;
			memcpy((void *)exc, ipkdblow, size);
#endif
			break;
#endif /* DDB || IPKDB || KGDB */
		}
#if 0
		exc += roundup(size, 32);
#endif
	}

	/*
	 * Install a branch absolute to trap0 to force a panic.
	 */
	if ((uintptr_t)trap0 < 0x2000000) {
		*(volatile uint32_t *) 0 = 0x7c6802a6;
		*(volatile uint32_t *) 4 = 0x48000002 | (uintptr_t) trap0;
	}

	/*
	 * Get the cache sizes because install_extint calls __syncicache.
	 */
	cpu_probe_cache();

#define	MxSPR_MASK	0x7c1fffff
#define	MFSPR_MQ	0x7c0002a6
#define	MTSPR_MQ	0x7c0003a6
#define	MTSPR_IBAT0L	0x7c1183a6
#define	MTSPR_IBAT1L	0x7c1383a6
#define	NOP		0x60000000
#define	B		0x48000000
#define	TLBSYNC		0x7c00046c
#define	SYNC		0x7c0004ac
#ifdef PPC_OEA64_BRIDGE
#define	MFMSR_MASK	0xfc1fffff
#define	MFMSR		0x7c0000a6
#define	MTMSRD_MASK	0xfc1effff
#define	MTMSRD		0x7c000164
#define RLDICL_MASK	0xfc00001c
#define RLDICL		0x78000000
#define	RFID		0x4c000024
#define	RFI		0x4c000064
#endif

#ifdef ALTIVEC
#define	MFSPR_VRSAVE	0x7c0042a6
#define	MTSPR_VRSAVE	0x7c0043a6
	
	/*
	 * Try to set the VEC bit in the MSR.  If it doesn't get set, we are
	 * not on a AltiVec capable processor.
	 */
	__asm volatile (
	    "mfmsr %0; oris %1,%0,%2@h; mtmsr %1; isync; "
		"mfmsr %1; mtmsr %0; isync"
	    :	"=r"(msr), "=r"(scratch)
	    :	"J"(PSL_VEC));

	/*
	 * If we aren't on an AltiVec capable processor, we need to zap any of
	 * the sequences we save/restore the VRSAVE SPR into NOPs.
	 */
	if (scratch & PSL_VEC) {
		cpu_altivec = 1;
	} else {
		for (int *ip = trapstart; ip < trapend; ip++) {
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

	/* XXX It would seem like this code could be elided ifndef 601, but
	 * doing so breaks my power3 machine.
	 */
	/*
	 * If we aren't on a MPC601 processor, we need to zap any of the
	 * sequences we save/restore the MQ SPR into NOPs, and skip over the
	 * sequences where we zap/restore BAT registers on kernel exit/entry.
	 */
	if (cpuvers != MPC601) {
		for (int *ip = trapstart; ip < trapend; ip++) {
			if ((ip[0] & MxSPR_MASK) == MFSPR_MQ) {
				ip[0] = NOP;	/* mfspr */
				ip[1] = NOP;	/* stw */
			} else if ((ip[0] & MxSPR_MASK) == MTSPR_MQ) {
				ip[-1] = NOP;	/* lwz */
				ip[0] = NOP;	/* mtspr */
			} else if ((ip[0] & MxSPR_MASK) == MTSPR_IBAT0L) {
				if ((ip[1] & MxSPR_MASK) == MTSPR_IBAT1L)
					ip[-1] = B | 0x14;	/* li */
				else
					ip[-4] = B | 0x24;	/* lis */
			}
		}
	}

#ifdef PPC_OEA64_BRIDGE
	if ((oeacpufeat & OEACPU_64_BRIDGE) == 0) {
		for (int *ip = (int *)exc_base;
		     (uintptr_t)ip <= exc_base + EXC_LAST;
		     ip++) {
			if ((ip[0] & MFMSR_MASK) == MFMSR
			    && (ip[1] & RLDICL_MASK) == RLDICL
			    && (ip[2] & MTMSRD_MASK) == MTMSRD) {
				*ip++ = NOP;
				*ip++ = NOP;
				ip[0] = NOP;
			} else if (*ip == RFID) {
				*ip = RFI;
			}
		}

		/*
		 * Now replace each rfid instruction with a rfi instruction.
		 */
		for (int *ip = trapstart; ip < trapend; ip++) {
			if ((ip[0] & MFMSR_MASK) == MFMSR
			    && (ip[1] & RLDICL_MASK) == RLDICL
			    && (ip[2] & MTMSRD_MASK) == MTMSRD) {
				*ip++ = NOP;
				*ip++ = NOP;
				ip[0] = NOP;
			} else if (*ip == RFID) {
				*ip = RFI;
			}
		}
	}
#endif /* PPC_OEA64_BRIDGE */

	/*
	 * Sync the changed instructions.
	 */
	__syncicache((void *) trapstart,
	    (uintptr_t) trapend - (uintptr_t) trapstart);
	__syncicache(dsitrap_fix_dbat4, 16);
	__syncicache(dsitrap_fix_dbat7, 8);
#ifdef PPC_OEA601

	/*
	 * If we are on a MPC601 processor, we need to zap any tlbsync
	 * instructions into sync.  This differs from the above in
	 * examing all kernel text, as opposed to just the exception handling.
	 * We sync the icache on every instruction found since there are
	 * only very few of them.
	 */
	if (cpuvers == MPC601) {
		extern int kernel_text[], etext[];
		int *ip;

		for (ip = kernel_text; ip < etext; ip++) {
			if (*ip == TLBSYNC) {
				*ip = SYNC;
				__syncicache(ip, sizeof(*ip));
			}
		}
	}
#endif /* PPC_OEA601 */

        /*
	 * Configure a PSL user mask matching this processor.
	 * Don't allow to set PSL_FP/PSL_VEC, since that will affect PCU.
 	 */
	cpu_psluserset = PSL_EE | PSL_PR | PSL_ME | PSL_IR | PSL_DR | PSL_RI;
	cpu_pslusermod = PSL_FE0 | PSL_FE1 | PSL_LE | PSL_SE | PSL_BE;
#ifdef PPC_OEA601
	if (cpuvers == MPC601) {
		cpu_psluserset &= PSL_601_MASK;
		cpu_pslusermod &= PSL_601_MASK;
	}
#endif
#ifdef PPC_HIGH_VEC
	cpu_psluserset |= PSL_IP;	/* XXX ok? */
#endif

	/*
	 * external interrupt handler install
	 */
	if (handler)
		oea_install_extint(handler);

	__syncicache((void *)exc_base, EXC_LAST + 0x100);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
#ifdef PPC_OEA
	__asm volatile ("sync; mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
	    : "=r"(scratch)
	    : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));
#endif

	/*
	 * Let's take all the indirect calls via our stubs and patch 
	 * them to be direct calls.
	 */
	cpu_fixup_stubs();

	KASSERT(curcpu() == ci);
}

#ifdef PPC_OEA601
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

	/*
	 * XXX Setting segment register 0xf on my powermac 7200
	 * wedges machine so set later in pmap.c
	 */
	/*
	__asm volatile ("mtsrin %0,%1"
	    ::	"r"(iosrtable[i]),
		"r"(pa));
	*/
}
#endif /* PPC_OEA601 */

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
#define	DBAT_SET(n, batl, batu)				\
	do {						\
		mtspr(SPR_DBAT##n##L, (batl));		\
		mtspr(SPR_DBAT##n##U, (batu));		\
	} while (/*CONSTCOND*/ 0)
#define	DBAT_RESET(n)	DBAT_SET(n, 0, 0)
#define	DBATU_GET(n)	mfspr(SPR_DBAT##n##U)
#define	IBAT_SET(n, batl, batu)				\
	do {						\
		mtspr(SPR_IBAT##n##L, (batl));		\
		mtspr(SPR_IBAT##n##U, (batu));		\
	} while (/*CONSTCOND*/ 0)
#define	IBAT_RESET(n)	IBAT_SET(n, 0, 0)
	
void
oea_iobat_add(paddr_t pa, register_t len)
{
	static int z = 1;
	const u_int n = BAT_BL_TO_SIZE(len) / BAT_BL_TO_SIZE(BAT_BL_8M);
	const u_int i = BAT_VA2IDX(pa) & -n; /* in case pa was in the middle */
	const int after_bat3 = (oeacpufeat & OEACPU_HIGHBAT) ? 4 : 8;

	KASSERT(len >= BAT_BL_8M);

	/*
	 * If the caller wanted a bigger BAT than the hardware supports,
	 * split it into smaller BATs.
	 */
	if (len > BAT_BL_256M && (oeacpufeat & OEACPU_XBSEN) == 0) {
		u_int xn = BAT_BL_TO_SIZE(len) >> 28;
		while (xn-- > 0) {
			oea_iobat_add(pa, BAT_BL_256M);
			pa += 0x10000000;
		}
		return;
	} 

	const register_t batl = BATL(pa, BAT_I|BAT_G, BAT_PP_RW);
	const register_t batu = BATU(pa, len, BAT_Vs);

	for (u_int j = 0; j < n; j++) {
		battable[i + j].batl = batl;
		battable[i + j].batu = batu;
	}

	/*
	 * Let's start loading the BAT registers.
	 */
	switch (z) {
	case 1:
		DBAT_SET(1, batl, batu);
		z = 2;
		break;
	case 2:
		DBAT_SET(2, batl, batu);
		z = 3;
		break;
	case 3:
		DBAT_SET(3, batl, batu);
		z = after_bat3;			/* no highbat, skip to end */
		break;
	case 4:
		DBAT_SET(4, batl, batu);
		z = 5;
		break;
	case 5:
		DBAT_SET(5, batl, batu);
		z = 6;
		break;
	case 6:
		DBAT_SET(6, batl, batu);
		z = 7;
		break;
	case 7:
		DBAT_SET(7, batl, batu);
		z = 8;
		break;
	default:
		break;
	}
}

void
oea_iobat_remove(paddr_t pa)
{
	const u_int i = BAT_VA2IDX(pa);

	if (!BAT_VA_MATCH_P(battable[i].batu, pa) ||
	    !BAT_VALID_P(battable[i].batu, PSL_PR))
		return;
	const int n =
	    __SHIFTOUT(battable[i].batu, (BAT_XBL|BAT_BL) & ~BAT_BL_8M) + 1;
	KASSERT((n & (n-1)) == 0);	/* power of 2 */
	KASSERT((i & (n-1)) == 0);	/* multiple of n */

	memset(&battable[i], 0, n*sizeof(battable[0]));

	const int maxbat = oeacpufeat & OEACPU_HIGHBAT ? 8 : 4;
	for (u_int k = 1 ; k < maxbat; k++) {
		register_t batu;
		switch (k) {
		case 1:
			batu = DBATU_GET(1);
			if (BAT_VA_MATCH_P(batu, pa) &&
			    BAT_VALID_P(batu, PSL_PR))
				DBAT_RESET(1);
			break;
		case 2:
			batu = DBATU_GET(2);
			if (BAT_VA_MATCH_P(batu, pa) &&
			    BAT_VALID_P(batu, PSL_PR))
				DBAT_RESET(2);
			break;
		case 3:
			batu = DBATU_GET(3);
			if (BAT_VA_MATCH_P(batu, pa) &&
			    BAT_VALID_P(batu, PSL_PR))
				DBAT_RESET(3);
			break;
		case 4:
			batu = DBATU_GET(4);
			if (BAT_VA_MATCH_P(batu, pa) &&
			    BAT_VALID_P(batu, PSL_PR))
				DBAT_RESET(4);
			break;
		case 5:
			batu = DBATU_GET(5);
			if (BAT_VA_MATCH_P(batu, pa) &&
			    BAT_VALID_P(batu, PSL_PR))
				DBAT_RESET(5);
			break;
		case 6:
			batu = DBATU_GET(6);
			if (BAT_VA_MATCH_P(batu, pa) &&
			    BAT_VALID_P(batu, PSL_PR))
				DBAT_RESET(6);
			break;
		case 7:
			batu = DBATU_GET(7);
			if (BAT_VA_MATCH_P(batu, pa) &&
			    BAT_VALID_P(batu, PSL_PR))
				DBAT_RESET(7);
			break;
		default:
			break;
		}
	}
}

void
oea_batinit(paddr_t pa, ...)
{
	struct mem_region *allmem, *availmem, *mp;
	register_t msr = mfmsr();
	va_list ap;
#ifdef PPC_OEA601
	unsigned int cpuvers;

	cpuvers = mfpvr() >> 16;
#endif /* PPC_OEA601 */

	/*
	 * we need to call this before zapping BATs so OF calls work
	 */
	mem_regions(&allmem, &availmem);

	/*
	 * Initialize BAT registers to unmapped to not generate
	 * overlapping mappings below.
	 *
	 * The 601's implementation differs in the Valid bit being situated
	 * in the lower BAT register, and in being a unified BAT only whose
	 * four entries are accessed through the IBAT[0-3] SPRs.
	 *
	 * Also, while the 601 does distinguish between supervisor/user
	 * protection keys, it does _not_ distinguish between validity in
	 * supervisor/user mode.
	 */
	if ((msr & (PSL_IR|PSL_DR)) == 0) {
#ifdef PPC_OEA601
		if (cpuvers == MPC601) {
			__asm volatile ("mtibatl 0,%0" :: "r"(0));
			__asm volatile ("mtibatl 1,%0" :: "r"(0));
			__asm volatile ("mtibatl 2,%0" :: "r"(0));
			__asm volatile ("mtibatl 3,%0" :: "r"(0));
		} else
#endif /* PPC_OEA601 */
		{
			DBAT_RESET(0); IBAT_RESET(0);
			DBAT_RESET(1); IBAT_RESET(1);
			DBAT_RESET(2); IBAT_RESET(2);
			DBAT_RESET(3); IBAT_RESET(3);
			if (oeacpufeat & OEACPU_HIGHBAT) {
				DBAT_RESET(4); IBAT_RESET(4);
				DBAT_RESET(5); IBAT_RESET(5);
				DBAT_RESET(6); IBAT_RESET(6);
				DBAT_RESET(7); IBAT_RESET(7);

				/*
				 * Change the first instruction to branch to
				 * dsitrap_fix_dbat6
				 */
				dsitrap_fix_dbat4[0] &= ~0xfffc;
				dsitrap_fix_dbat4[0]
				    += (uintptr_t)dsitrap_fix_dbat6
				     - (uintptr_t)&dsitrap_fix_dbat4[0];

				/*
				 * Change the second instruction to branch to
				 * dsitrap_fix_dbat5 if bit 30 (aka bit 1) is
				 * true.
				 */
				dsitrap_fix_dbat4[1] = 0x419e0000
				    + (uintptr_t)dsitrap_fix_dbat5
				    - (uintptr_t)&dsitrap_fix_dbat4[1];

				/*
				 * Change it to load dbat4 instead of dbat2
				 */
				dsitrap_fix_dbat4[2] = 0x7fd88ba6;
				dsitrap_fix_dbat4[3] = 0x7ff98ba6;

				/*
				 * Change it to load dbat7 instead of dbat3
				 */
				dsitrap_fix_dbat7[0] = 0x7fde8ba6;
				dsitrap_fix_dbat7[1] = 0x7fff8ba6;
			}
		}
	}

	/*
	 * Set up BAT to map physical memory
	 */
#ifdef PPC_OEA601
	if (cpuvers == MPC601) {
		int i;
		
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
		__asm volatile ("mtibatu 0,%1; mtibatl 0,%0"
		    :: "r"(battable[0x00000000 >> 23].batl),
		       "r"(battable[0x00000000 >> 23].batu));
		__asm volatile ("mtibatu 1,%1; mtibatl 1,%0"
		    :: "r"(battable[0x00800000 >> 23].batl),
		       "r"(battable[0x00800000 >> 23].batu));
		__asm volatile ("mtibatu 2,%1; mtibatl 2,%0"
		    :: "r"(battable[0x01000000 >> 23].batl),
		       "r"(battable[0x01000000 >> 23].batu));
		__asm volatile ("mtibatu 3,%1; mtibatl 3,%0"
		    :: "r"(battable[0x01800000 >> 23].batl),
		       "r"(battable[0x01800000 >> 23].batu));
	}
#endif /* PPC_OEA601 */
	
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
#ifdef PPC_OEA601
	if (cpuvers == MPC601) {
		while (pa != 0) {
			register_t len = va_arg(ap, register_t);
			mpc601_ioseg_add(pa, len);
			pa = va_arg(ap, paddr_t);
		}
	} else
#endif
	{
		while (pa != 0) {
			register_t len = va_arg(ap, register_t);
			oea_iobat_add(pa, len);
			pa = va_arg(ap, paddr_t);
		}
	}

	va_end(ap);

	/*
	 * Set up battable to map all RAM regions.
	 */
#ifdef PPC_OEA601
	if (cpuvers == MPC601) {
		for (mp = allmem; mp->size; mp++) {
			paddr_t paddr = mp->start & 0xff800000;
			paddr_t end = mp->start + mp->size;

			do {
				u_int ix = paddr >> 23;

				battable[ix].batl =
				    BATL601(paddr, BAT601_BSM_8M, BAT601_V);
				battable[ix].batu =
				    BATU601(paddr, BAT601_M, BAT601_Ku, BAT601_PP_NONE);
				paddr += (1 << 23);
			} while (paddr < end);
		}
	} else
#endif
	{
		const register_t bat_inc = BAT_IDX2VA(1);
		for (mp = allmem; mp->size; mp++) {
			paddr_t paddr = mp->start & -bat_inc;
			paddr_t end = roundup2(mp->start + mp->size, bat_inc);

			/*
			 * If the next entries are adjacent, merge them
			 * into this one
			 */
			while (mp[1].size && end == (mp[1].start & -bat_inc)) {
				mp++;
				end = roundup2(mp->start + mp->size, bat_inc);
			}

			while (paddr < end) {
				register_t bl = (oeacpufeat & OEACPU_XBSEN
				    ? BAT_BL_2G
				    : BAT_BL_256M);
				psize_t size = BAT_BL_TO_SIZE(bl);
				u_int n = BAT_VA2IDX(size);
				u_int i = BAT_VA2IDX(paddr);

				while ((paddr & (size - 1))
				    || paddr + size > end) {
					size >>= 1;
					bl = (bl >> 1) & (BAT_XBL|BAT_BL);
					n >>= 1;
				}

				KASSERT(size >= bat_inc);
				KASSERT(n >= 1);
				KASSERT(bl >= BAT_BL_8M);

				register_t batl = BATL(paddr, BAT_M, BAT_PP_RW);
				register_t batu = BATU(paddr, bl, BAT_Vs);

				for (; n-- > 0; i++) {
					battable[i].batl = batl;
					battable[i].batu = batu;
				}
				paddr += size;
			}
		}
		/*
		 * Set up BAT0 to only map the lowest area.
		 */
		__asm volatile ("mtibatl 0,%0; mtibatu 0,%1;"
				  "mtdbatl 0,%0; mtdbatu 0,%1;"
		    ::	"r"(battable[0].batl), "r"(battable[0].batu));
	}
}
#endif /* PPC_OEA || PPC_OEA64_BRIDGE */

void
oea_install_extint(void (*handler)(void))
{
	extern int extint[], extsize[];
	extern int extint_call[];
	uintptr_t offset = (uintptr_t)handler - (uintptr_t)extint_call;
#ifdef PPC_HIGH_VEC
	const uintptr_t exc_exi_base = EXC_HIGHVEC + EXC_EXI;
#else
	const uintptr_t exc_exi_base = EXC_EXI;
#endif
	int omsr, msr;

#ifdef	DIAGNOSTIC
	if (offset > 0x1ffffff)
		panic("install_extint: %p too far away (%#lx)", handler,
		    (unsigned long) offset);
#endif
	__asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
	    :	"=r" (omsr), "=r" (msr)
	    :	"K" ((u_short)~PSL_EE));
	extint_call[0] = (extint_call[0] & 0xfc000003) | offset;
	__syncicache((void *)extint_call, sizeof extint_call[0]);
	memcpy((void *)exc_exi_base, extint, (size_t)extsize);
#ifdef PPC_OEA64_BRIDGE
	if ((oeacpufeat & OEACPU_64_BRIDGE) == 0) {
		for (int *ip = (int *)exc_exi_base;
		     (uintptr_t)ip <= exc_exi_base + (size_t)extsize;
		     ip++) {
			if ((ip[0] & MFMSR_MASK) == MFMSR
			    && (ip[1] & RLDICL_MASK) == RLDICL
			    && (ip[2] & MTMSRD_MASK) == MTMSRD) {
				*ip++ = NOP;
				*ip++ = NOP;
				ip[0] = NOP;
			} else if (*ip == RFID) {
				*ip = RFI;
			}
		}
	}
#endif
	__syncicache((void *)exc_exi_base, (size_t)extsize);

	__asm volatile ("mtmsr %0" :: "r"(omsr));
}

/*
 * Machine dependent startup code.
 */
void
oea_startup(const char *model)
{
	uintptr_t sz;
	void *v;
	vaddr_t minaddr, maxaddr;
	char pbuf[9], mstr[128];

	KASSERT(curcpu() != NULL);
	KASSERT(lwp0.l_cpu != NULL);
	KASSERT(curcpu()->ci_idepth == -1);

	sz = round_page(MSGBUFSIZE);
#ifdef MSGBUFADDR
	v = (void *) MSGBUFADDR;
#else
	/*
	 * If the msgbuf is not in segment 0, allocate KVA for it and access
	 * it via mapped pages.  [This prevents unneeded BAT switches.]
	 */
	v = (void *) msgbuf_paddr;
	if (msgbuf_paddr + sz > SEGMENT_LENGTH) {
		u_int i;

		minaddr = 0;
		if (uvm_map(kernel_map, &minaddr, sz,
				NULL, UVM_UNKNOWN_OFFSET, 0,
				UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE,
				    UVM_INH_NONE, UVM_ADV_NORMAL, 0)) != 0)
			panic("startup: cannot allocate VM for msgbuf");
		v = (void *)minaddr;
		for (i = 0; i < sz; i += PAGE_SIZE) {
			pmap_kenter_pa(minaddr + i, msgbuf_paddr + i,
			    VM_PROT_READ|VM_PROT_WRITE, 0);
		}
		pmap_update(pmap_kernel());
	}
#endif
	initmsgbuf(v, sz);

	printf("%s%s", copyright, version);
	if (model != NULL)
		printf("Model: %s\n", model);
	cpu_identify(mstr, sizeof(mstr));
	cpu_setmodel("%s", mstr);

	format_bytes(pbuf, sizeof(pbuf), ctob((u_int)physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Allocate away the pages that map to 0xDEA[CDE]xxxx.  Do this after
	 * the bufpages are allocated in case they overlap since it's not
	 * fatal if we can't allocate these.
	 */
	if (KERNEL_SR == 13 || KERNEL2_SR == 14) {
		int error;
		minaddr = 0xDEAC0000;
		error = uvm_map(kernel_map, &minaddr, 0x30000,
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,  
				UVM_ADV_NORMAL, UVM_FLAG_FIXED));
		if (error != 0 || minaddr != 0xDEAC0000)
			printf("oea_startup: failed to allocate DEAD "
			    "ZONE: error=%d\n", error);
	}
 
	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, false, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
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
 * Convert kernel VA to physical address
 */
paddr_t
kvtop(void *addr)
{
	vaddr_t va;
	paddr_t pa;
	uintptr_t off;
	extern char end[];

	if (addr < (void *)end)
		return (paddr_t)addr;

	va = trunc_page((vaddr_t)addr);
	off = (uintptr_t)addr - va;

	if (pmap_extract(pmap_kernel(), va, &pa) == false) {
		/*printf("kvtop: zero page frame (va=0x%x)\n", addr);*/
		return (paddr_t)addr;
	}

	return(pa + off);
}

/*
 * Allocate vm space and mapin the I/O address
 */
void *
mapiodev(paddr_t pa, psize_t len, bool prefetchable)
{
	paddr_t faddr;
	vaddr_t taddr, va;
	int off;

	faddr = trunc_page(pa);
	off = pa - faddr;
	len = round_page(off + len);
	va = taddr = uvm_km_alloc(kernel_map, len, 0, UVM_KMF_VAONLY);

	if (va == 0)
		return NULL;

	for (; len > 0; len -= PAGE_SIZE) {
		pmap_kenter_pa(taddr, faddr, VM_PROT_READ | VM_PROT_WRITE,
		    (prefetchable ? PMAP_MD_PREFETCHABLE : PMAP_NOCACHE));
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
	return (void *)(va + off);
}

void
unmapiodev(vaddr_t va, vsize_t len)
{
	paddr_t faddr;

	if (! va)
		return;

	faddr = trunc_page(va);
	len = round_page(va - faddr + len);

	pmap_kremove(faddr, len);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, faddr, len, UVM_KMF_VAONLY);
}

void
trap0(void *lr)
{
	panic("call to null-ptr from %p", lr);
}
