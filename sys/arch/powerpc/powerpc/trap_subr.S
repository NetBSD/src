/*	$NetBSD: trap_subr.S,v 1.86 2022/05/18 13:56:32 andvar Exp $	*/

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

/*
 * NOTICE: This is not a standalone file.  to use it, #include it in
 * your port's locore.S, like so:
 *
 *	#include <powerpc/powerpc/trap_subr.S>
 */

#ifdef _KERNEL_OPT
#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ppcarch.h"
#endif

/* LINTSTUB: include <sys/param.h> */
/* LINTSTUB: include <powerpc/oea/bat.h> */

#ifdef ALTIVEC
#define	SAVE_VRSAVE(tf,b)						\
	mfspr	b,SPR_VRSAVE;						\
	stint	b,FRAME_VRSAVE(tf);

#define RESTORE_VRSAVE(tf,b)						\
	ldint	b,FRAME_VRSAVE(tf);					\
	mtspr	SPR_VRSAVE,b;
#else
#define SAVE_VRSAVE(tf,b)
#define RESTORE_VRSAVE(tf,b)
#endif

#if defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#define	RFI	rfid
#else
#define	RFI	rfi
#endif /* PPC_OEA64 || PPC_OEA64_BRIDGE*/

#if defined (PPC_OEA64_BRIDGE)
#define ENABLE_64BIT_BRIDGE(t0)						\
	mfmsr	t0;							\
	clrldi	t0,t0,1;						\
	mtmsrd	t0;
#else
#define ENABLE_64BIT_BRIDGE(t0)
#endif /* PPC_OEA64_BRIDGE */

#if defined(PPC_OEA64)
/*
 * User segment table is loaded through a pointer to the current pmap.
 */
#define RESTORE_USER_SRS(t0,t1)						\
	GET_CPUINFO(t0);						\
	ldptr	t0,CI_CURPM(t0);					\
	ldreg	t0,PM_STEG(t0);						\
	mtasr	t0

/*
 * Kernel segment table is loaded directly from kernel_pmap_
 */
#define RESTORE_KERN_SRS(t0,t1)						\
	lis	t0,_C_LABEL(kernel_pmap_)@ha;				\
	ldreg	t0,_C_LABEL(kernel_pmap_)+PM_STEG@l(t0);		\
	mtasr	t0

#elif defined(PPC_MPC8XX)

/*
 * PPC_MPC8XX don't have SRs to load
 */
#define RESTORE_USER_SRS(t0,t1)
#define RESTORE_KERN_SRS(t0,t1)

#else /* not OEA64 */

/*
 * Restore segment registers from array.
 */
#define	RESTORE_SRS(pmap,sr)	mtsr	0,sr; \
	ldreg	sr,4(pmap);	mtsr	1,sr; \
	ldreg	sr,8(pmap);	mtsr	2,sr; \
	ldreg	sr,12(pmap);	mtsr	3,sr; \
	ldreg	sr,16(pmap);	mtsr	4,sr; \
	ldreg	sr,20(pmap);	mtsr	5,sr; \
	ldreg	sr,24(pmap);	mtsr	6,sr; \
	ldreg	sr,28(pmap);	mtsr	7,sr; \
	ldreg	sr,32(pmap);	mtsr	8,sr; \
	ldreg	sr,36(pmap);	mtsr	9,sr; \
	ldreg	sr,40(pmap);	mtsr	10,sr; \
	ldreg	sr,44(pmap);	mtsr	11,sr; \
	ldreg	sr,48(pmap);	mtsr	12,sr; \
	ldreg	sr,52(pmap);	mtsr	13,sr; \
	ldreg	sr,56(pmap);	mtsr	14,sr; \
	ldreg	sr,60(pmap);	mtsr	15,sr; isync;

/*
 * User SRs are loaded through a pointer to the current pmap.
 * Note: oea_init() relies on the 601 instruction sequence.
 */
#define RESTORE_USER_SRS(pmap,sr)					\
	GET_CPUINFO(pmap);						\
	ldptr	pmap,CI_CURPM(pmap);					\
	ldregu	sr,PM_SR(pmap);						\
	RESTORE_SRS(pmap,sr);						\
	/* Obliterate BATs on 601; reuse temporary registers. */	\
	li	sr,0;							\
	mtibatl	0,sr;							\
	mtibatl	1,sr;							\
	mtibatl	2,sr;							\
	mtibatl	3,sr

/*
 * Kernel SRs are loaded directly from kernel_pmap_.
 * Note: oea_init() relies on the 601 instruction sequence.
 */
#define RESTORE_KERN_SRS(pmap,sr)					\
	lis	pmap,_C_LABEL(kernel_pmap_)@ha;				\
	ldregu	sr,_C_LABEL(kernel_pmap_)+PM_SR@l(pmap);		\
	RESTORE_SRS(pmap,sr);						\
	/* Restore fixed BATs on 601; reuse temporary registers. */	\
	lis	pmap,_C_LABEL(battable)@ha;				\
	ldregu	sr,_C_LABEL(battable)@l(pmap);				\
	mtibatu 0,sr;							\
	ldreg	sr,4(pmap);	mtibatl 0,sr;				\
	ldreg	sr,8(pmap);	mtibatu 1,sr;				\
	ldreg	sr,12(pmap);	mtibatl 1,sr
#endif /* (PPC_OEA64) */

/*
 * Save/restore MPC601 MQ register.
 * Note: oea_init() relies on this instruction sequence.
 */
#if defined(PPC_OEA601)
#define	SAVE_MQ(tf,b)							\
	mfspr	b,SPR_MQ;						\
	streg	b,FRAME_MQ(tf);

#define	RESTORE_MQ(tf,b)						\
	ldreg	b,FRAME_MQ(tf);						\
	mtspr	SPR_MQ,b;
#else
#define	SAVE_MQ(tf,b)
#define	RESTORE_MQ(tf,b)
#endif

/*
 * This code gets copied to all the trap vectors
 * (except ISI/DSI, ALI, the interrupts).
 */

/* LINTSTUB: Var: int trapcode[1], trapsize[1]; */
	.text
	.globl	_C_LABEL(trapcode),_C_LABEL(trapsize)
_C_LABEL(trapcode):
	mtsprg1	%r1			/* save SP */
	ENABLE_64BIT_BRIDGE(%r1)
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)	/* free r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)	/* free r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)	/* free r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)	/* free r31 */
	mfsprg1	%r1			/* restore SP */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
/* Test whether we already had PR set */
	mfsrr1	%r31
	mtcr	%r31
#if defined(DISTANT_KERNEL)
	lis	%r31,s_trap@ha
	addi	%r31,%r31,s_trap@l
	mtlr	%r31
	blrl
#else
	bla	s_trap
#endif
_C_LABEL(trapsize) = .-_C_LABEL(trapcode)

/*
 * For ALI: has to save DSISR and DAR
 * Also used as dsitrap for BATless cpus.
 */
/* LINTSTUB: Var: int alicode[1], alisize[1]; */
	.globl	_C_LABEL(alitrap),_C_LABEL(alisize)
_C_LABEL(alitrap):
	mtsprg1	%r1			/* save SP */
	ENABLE_64BIT_BRIDGE(%r1)
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1) /* save r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1) /* save r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1) /* save r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1) /* save r31 */
	mfdar	%r30
	mfdsisr	%r31
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_DAR)(%r1)	/* save dar */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_DSISR)(%r1)	/* save dsisr */
	mfsprg1	%r1			/* restore SP */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
/* Test whether we already had PR set */
	mfsrr1	%r31
	mtcr	%r31
#if defined(DISTANT_KERNEL)
	lis	%r31,s_trap@ha
	addi	%r31,%r31,s_trap@l
	mtlr	%r31
	blrl
#else
	bla	s_trap
#endif
_C_LABEL(alisize) = .-_C_LABEL(alitrap)

#if !defined(PPC_MPC8XX)
/*
 * Similar to the above for DSI
 * Has to handle BAT spills
 * and standard pagetable spills
 */
/* LINTSTUB: Var: int dsicode[1], dsisize[1]; */
	.globl	_C_LABEL(dsitrap),_C_LABEL(dsisize)
_C_LABEL(dsitrap):
	mtsprg1	%r1
	ENABLE_64BIT_BRIDGE(%r1)
	GET_CPUINFO(%r1)
	streg	%r28,(CI_DISISAVE+CPUSAVE_R28)(%r1)	/* save r28 */
	streg	%r29,(CI_DISISAVE+CPUSAVE_R29)(%r1)	/* save r29 */
	streg	%r30,(CI_DISISAVE+CPUSAVE_R30)(%r1)	/* save r30 */
	streg	%r31,(CI_DISISAVE+CPUSAVE_R31)(%r1)	/* save r31 */
	mfsprg1	%r1
	mfcr	%r29			/* save CR */
	mfsrr1	%r31			/* test kernel mode */
	mtcr	%r31
#if !defined(PPC_MPC8XX)
	mfxer	%r30			/* save XER */
	mtsprg2	%r30			/* in SPRG2 */
	bt	MSR_PR,1f		/* branch if PSL_PR is set */
	mfdar	%r31			/* get fault address */
	rlwinm	%r31,%r31,3+(32-BAT_ADDR_SHIFT),BAT_ADDR_SHIFT-3,28
					/* get segment * 8 */

	/* Get address of this CPU's current battable */
	GET_CPUINFO(%r30)
	ldreg	%r30,CI_BATTABLE(%r30)

	/* Add offset to the slot we care about. */
	add	%r31,%r31,%r30

	/* get batu */
	ldreg	%r30,0(%r31)
	mtcr	%r30
	bf	30,1f			/* branch if supervisor valid is
					   false */
	/* get batl */
	ldreg	%r31,SZREG(%r31)
/* We randomly use the highest two bat registers here */
	mftb	%r28
	mtcr	%r28
	.globl	dsitrap_fix_dbat4, dsitrap_fix_dbat5
	.globl	dsitrap_fix_dbat6, dsitrap_fix_dbat7
dsitrap_fix_dbat4:
	bt	31,3f
	/*
	 * If we are running on a CPU that has HIGHBAT, these will be replaced
	 * by instructions to test bit 30 (aka bit 1 for normal people) and if
	 * set skip ahead to 5f (4 instructions),  follored by instructions to
	 * update BAT4.
	 */
	mtspr	SPR_DBAT2U,%r30		/*	bt	30,dsitrap_fix_dbat5 */
	mtspr	SPR_DBAT2L,%r31		/*	mtspr	SPR_DBAT4U,%r30 */
	b	8f			/*	mtspr	SPR_DBAT4L,%r31 */	
	b	8f			/*	do not remove */
dsitrap_fix_dbat5:
	mtspr	SPR_DBAT5U,%r30
	mtspr	SPR_DBAT5L,%r31
	b	8f
dsitrap_fix_dbat6:
	bt	30,3f
	mtspr	SPR_DBAT6U,%r30
	mtspr	SPR_DBAT6L,%r31
	b	8f
3:
dsitrap_fix_dbat7:
	/*
	 * If we are running on a CPU that has HIGHBAT, these will be replaced
	 * by instructions to update BAT7.
	 */
	mtspr	SPR_DBAT3U,%r30		/*	mtspr	SPR_DBAT7U,%r30 */
	mtspr	SPR_DBAT3L,%r31		/*	mtspr	SPR_DBAT7L,%r31 */
8:
	mfsprg2	%r30			/* restore XER */
	mtxer	%r30
	mtcr	%r29			/* restore CR */
	mtsprg1	%r1
	GET_CPUINFO(%r1)
	ldreg	%r28,(CI_DISISAVE+CPUSAVE_R28)(%r1)	/* restore r28 */
	ldreg	%r29,(CI_DISISAVE+CPUSAVE_R29)(%r1)	/* restore r29 */
	ldreg	%r30,(CI_DISISAVE+CPUSAVE_R30)(%r1)	/* restore r30 */
	ldreg	%r31,(CI_DISISAVE+CPUSAVE_R31)(%r1)	/* restore r31 */
	mfsprg1	%r1
	RFI				/* return to trapped code */
1:
#endif	/* !PPC_MPC8XX */
	mflr	%r28			/* save LR */
	mtsprg1	%r1			/* save SP */
#if defined(DISTANT_KERNEL)
	lis	%r31,disitrap@ha
	addi	%r31,%r31,disitrap@l
	mtlr	%r31
	blrl
#else
	bla	disitrap
#endif
_C_LABEL(dsisize) = .-_C_LABEL(dsitrap)
#endif /* !PPC_MPC8XX */

#if defined(PPC_OEA601)
/*
 * Dedicated MPC601 version of the above.
 * Considers different BAT format and combined implementation
 * (being addressed as I-BAT).
 */
/* LINTSTUB: Var: int dsi601code[1], dsi601size[1]; */
	.globl	_C_LABEL(dsi601trap),_C_LABEL(dsi601size)
_C_LABEL(dsi601trap):
	mtsprg1	%r1
	ENABLE_64BIT_BRIDGE(%r1)
	GET_CPUINFO(%r1)
	streg	%r28,(CI_DISISAVE+CPUSAVE_R28)(%r1)	/* save r28 */
	streg	%r29,(CI_DISISAVE+CPUSAVE_R29)(%r1)	/* save r29 */
	streg	%r30,(CI_DISISAVE+CPUSAVE_R30)(%r1)	/* save r30 */
	streg	%r31,(CI_DISISAVE+CPUSAVE_R31)(%r1)	/* save r31 */
	mfsprg1	%r1
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	mtsprg2	%r30			/* in SPRG2 */
	mfsrr1	%r31			/* test kernel mode */
	mtcr	%r31
	bt	MSR_PR,1f		/* branch if PSL_PR is set */
	mfdar	%r31			/* get fault address */
	rlwinm	%r31,%r31,12,20,28	/* get "segment" battable offset */

	/* Get address of this CPU's current battable */
	GET_CPUINFO(%r30)
	ldreg	%r30,CI_BATTABLE(%r30)

	/* Add offset to the slot we care about. */
	add	%r31,%r31,%r30

	/* get batl */
	ldreg	%r30,SZREG(%r31)
	mtcr	%r30
	bf	25,1f			/* branch if Valid is false,
					   presently assumes supervisor only */

	/* get batu */
	ldreg	%r31,0(%r31)
/* We randomly use the highest two bat registers here */
	mfspr	%r28,SPR_RTCL_R
	andi.	%r28,%r28,128
	bne	2f
	mtibatu	2,%r31
	mtibatl	2,%r30
	b	3f
2:
	mtibatu	3,%r31
	mtibatl	3,%r30
3:
	mfsprg2	%r30			/* restore XER */
	mtxer	%r30
	mtcr	%r29			/* restore CR */
	mtsprg1	%r1
	GET_CPUINFO(%r1)
	ldreg	%r28,(CI_DISISAVE+CPUSAVE_R28)(%r1)	/* restore r28 */
	ldreg	%r29,(CI_DISISAVE+CPUSAVE_R29)(%r1)	/* restore r29 */
	ldreg	%r30,(CI_DISISAVE+CPUSAVE_R30)(%r1)	/* restore r30 */
	ldreg	%r31,(CI_DISISAVE+CPUSAVE_R31)(%r1)	/* restore r31 */
	mfsprg1	%r1
	RFI				/* return to trapped code */
1:
	mflr	%r28			/* save LR */
	mtsprg1	%r1
#if defined(DISTANT_KERNEL)
	lis	%r31,disitrap@ha
	addi	%r31,%r31,disitrap@l
	mtlr	%r31
	blrl
#else
	bla	disitrap
#endif
_C_LABEL(dsi601size) = .-_C_LABEL(dsi601trap)
#endif /* defined(PPC_OEA601) */

/*
 * This one for the external interrupt handler.
 */
/* LINTSTUB: Var: int extint[1], extsize[1]; */
	.globl	_C_LABEL(extint),_C_LABEL(extsize)
_C_LABEL(extint):
	mtsprg1	%r1			/* save SP */
	ENABLE_64BIT_BRIDGE(%r1)
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)	/* save r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)	/* save r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)	/* save r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)	/* save r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfsrr1	%r31
	mtcr	%r31
	mr	%r30,%r1
	mfsprg1	%r1			/* get old SP */
	bf	MSR_PR,1f		/* branch if PSL_PR is true */
	ldptr	%r1,CI_CURPCB(%r30)	/* get kernel stack */
	addi	%r1,%r1,USPACE-CALLFRAMELEN /* stack is top of user struct */
	RESTORE_KERN_SRS(%r30, %r31)
1:
#if defined(DISTANT_KERNEL)
	lis	%r31,extintr@ha
	addi	%r31,%r31,extintr@l
	mtlr	%r31
	blr
#else
	ba	extintr
#endif
_C_LABEL(extsize) = .-_C_LABEL(extint)

/*
 * And this one for the decrementer interrupt handler.
 */
/* LINTSTUB: Var: int decrint[1], decrsize[1]; */
	.globl	_C_LABEL(decrint),_C_LABEL(decrsize)
_C_LABEL(decrint):
	mtsprg1	%r1			/* save SP */
	ENABLE_64BIT_BRIDGE(%r1)
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)	/* save r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)	/* save r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)	/* save r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)	/* save r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfsrr1	%r31
	mtcr	%r31
	mr	%r30,%r1
	mfsprg1	%r1			/* yes, get old SP */
	bf	MSR_PR,1f		/* branch if PSL_PR is true */
	ldptr	%r1,CI_CURPCB(%r30)	/* get kernel stack */
	addi	%r1,%r1,USPACE-CALLFRAMELEN /* stack is top of user struct */
	RESTORE_KERN_SRS(%r30, %r31)
1:
#if defined(DISTANT_KERNEL)
	lis	%r31,decrintr@ha
	addi	%r31,%r31,decrintr@l
	mtlr	%r31
	blr
#else
	ba	decrintr
#endif
_C_LABEL(decrsize) = .-_C_LABEL(decrint)

#if !defined(PPC_OEA64) && !defined(PPC_MPC8XX)
/*
 * Now the tlb software load for 603 processors:
 * (Code essentially from the 603e User Manual, Chapter 5, but
 * corrected a lot.)
 */
/* LINTSTUB: Var: int tlbimiss[1], tlbimsize[1]; */
	.globl	_C_LABEL(tlbimiss),_C_LABEL(tlbimsize)
_C_LABEL(tlbimiss):
	mfspr	%r2,SPR_HASH1		/* get first pointer */
	li	%r1,8
	mfctr	%r0			/* save counter */
	mfspr	%r3,SPR_ICMP		/* get first compare value */
	addi	%r2,%r2,-8		/* predec pointer */
1:
	mtctr	%r1			/* load counter */
2:
	ldregu	%r1,8(%r2)		/* get next pte */
	cmplw	%r1,%r3			/* see if found pte */
	bdneq	2b			/* loop if not eq */
	bne	3f			/* not found */
	ldreg	%r1,4(%r2)		/* load tlb entry lower word */
	andi.	%r3,%r1,PTE_G		/* check G-bit */
	bne	4f			/* if guarded, take ISI */
	mtctr	%r0			/* restore counter */
	mfspr	%r0,SPR_IMISS		/* get the miss address for the tlbli */
	mfsrr1	%r3			/* get the saved cr0 bits */
	mtcrf	0x80,%r3		/* and restore */
	ori	%r1,%r1,PTE_REF		/* set the reference bit */
	mtspr	SPR_RPA,1		/* set the pte */
	srwi	%r1,%r1,8		/* get byte 7 of pte */
	tlbli	%r0			/* load the itlb */
	stb	%r1,6(%r2)		/* update page table */
	RFI

3:	/* not found in pteg */
	andi.	%r1,%r3,PTE_HID		/* have we already done second hash? */
	bne	5f
	mfspr	%r2,SPR_HASH2		/* get the second pointer */
	ori	%r3,%r3,PTE_HID		/* change the compare value */
	li	%r1,8
	addi	%r2,%r2,-8		/* predec pointer */
	b	1b
4:	/* guarded */
	mfsrr1	%r3
	andi.	%r2,%r3,0xffff		/* clean upper srr1 */
	oris	%r2,%r2,DSISR_PROTECT@h	/* set srr<4> to flag prot violation */
	b	6f
5:	/* not found anywhere */
	mfsrr1	%r3
	andi.	%r2,%r3,0xffff		/* clean upper srr1 */
	oris	%r2,%r2,DSISR_NOTFOUND@h /* set srr1<1> to flag pte not found */
6:
	mtctr	%r0			/* restore counter */
	mtsrr1	%r2
	mfmsr	%r0
	xoris	%r0,%r0,PSL_TGPR@h	/* flip the msr<tgpr> bit */
	mtcrf	0x80,%r3		/* restore cr0 */
	mtmsr	%r0			/* now with native gprs */
	isync
#if defined(PPC_HIGH_VEC)
	ba	EXC_HIGHVEC+EXC_ISI
#else
	ba	EXC_ISI
#endif
_C_LABEL(tlbimsize) = .-_C_LABEL(tlbimiss)

/* LINTSTUB: Var: int tlbdlmiss[1], tlbdlmsize[1]; */
	.globl	_C_LABEL(tlbdlmiss),_C_LABEL(tlbdlmsize)
_C_LABEL(tlbdlmiss):
	mfspr	%r2,SPR_HASH1		/* get first pointer */
	li	%r1,8
	mfctr	%r0			/* save counter */
	mfspr	%r3,SPR_DCMP		/* get first compare value */
	addi	%r2,%r2,-8		/* predec pointer */
1:
	mtctr	%r1			/* load counter */
2:
	ldregu	%r1,8(%r2)		/* get next pte */
	cmplw	%r1,%r3			/* see if found pte */
	bdneq	2b			/* loop if not eq */
	bne	3f			/* not found */
	ldreg	%r1,4(%r2)		/* load tlb entry lower word */
	mtctr	%r0			/* restore counter */
	mfspr	%r0,SPR_DMISS		/* get the miss address for the tlbld */
	mfsrr1	%r3			/* get the saved cr0 bits */
	mtcrf	0x80,%r3		/* and restore */
	ori	%r1,%r1,PTE_REF		/* set the reference bit */
	mtspr	SPR_RPA,%r1		/* set the pte */
	srwi	%r1,%r1,8		/* get byte 7 of pte */
	tlbld	%r0			/* load the dtlb */
	stb	%r1,6(%r2)		/* update page table */
	RFI

3:	/* not found in pteg */
	andi.	%r1,%r3,PTE_HID		/* have we already done second hash? */
	bne	5f
	mfspr	%r2,SPR_HASH2		/* get the second pointer */
	ori	%r3,%r3,PTE_HID		/* change the compare value */
	li	%r1,8
	addi	%r2,%r2,-8		/* predec pointer */
	b	1b
5:	/* not found anywhere */
	mfsrr1	%r3
	lis	%r1,DSISR_NOTFOUND@h	/* set dsisr<1> to flag pte not found */
	mtctr	%r0			/* restore counter */
	andi.	%r2,%r3,0xffff		/* clean upper srr1 */
	mtsrr1	%r2
	mtdsisr	%r1			/* load the dsisr */
	mfspr	%r1,SPR_DMISS		/* get the miss address */
	mtdar	%r1			/* put in dar */
	mfmsr	%r0
	xoris	%r0,%r0,PSL_TGPR@h	/* flip the msr<tgpr> bit */
	mtcrf	0x80,%r3		/* restore cr0 */
	mtmsr	%r0			/* now with native gprs */
	isync
#if defined(PPC_HIGH_VEC)
	ba	EXC_HIGHVEC+EXC_DSI
#else
	ba	EXC_DSI
#endif
_C_LABEL(tlbdlmsize) = .-_C_LABEL(tlbdlmiss)

/* LINTSTUB: Var: int tlbdsmiss[1], tlbdsmsize[1]; */
	.globl	_C_LABEL(tlbdsmiss),_C_LABEL(tlbdsmsize)
_C_LABEL(tlbdsmiss):
	mfspr	%r2,SPR_HASH1		/* get first pointer */
	li	%r1,8
	mfctr	%r0			/* save counter */
	mfspr	%r3,SPR_DCMP		/* get first compare value */
	addi	%r2,%r2,-8		/* predec pointer */
1:
	mtctr	%r1			/* load counter */
2:
	ldregu	%r1,8(%r2)		/* get next pte */
	cmplw	%r1,%r3			/* see if found pte */
	bdneq	2b			/* loop if not eq */
	bne	3f			/* not found */
	ldreg	%r1,4(%r2)		/* load tlb entry lower word */
	andi.	%r3,%r1,PTE_CHG		/* check the C-bit */
	beq	4f
5:
	mtctr	%r0			/* restore counter */
	mfspr	%r0,SPR_DMISS		/* get the miss address for the tlbld */
	mfsrr1	%r3			/* get the saved cr0 bits */
	mtcrf	0x80,%r3		/* and restore */
	mtspr	SPR_RPA,%r1		/* set the pte */
	tlbld	%r0			/* load the dtlb */
	RFI

3:	/* not found in pteg */
	andi.	%r1,%r3,PTE_HID		/* have we already done second hash? */
	bne	5f
	mfspr	%r2,SPR_HASH2		/* get the second pointer */
	ori	%r3,%r3,PTE_HID		/* change the compare value */
	li	%r1,8
	addi	%r2,%r2,-8		/* predec pointer */
	b	1b
4:	/* found, but C-bit = 0 */
	rlwinm.	%r3,%r1,30,0,1		/* test PP */
	bge-	7f
	andi.	%r3,%r1,1
	beq+	8f
9:	/* found, but protection violation (PP==00)*/
	mfsrr1	%r3
	lis	%r1,(DSISR_PROTECT|DSISR_STORE)@h
					/* indicate protection violation
					   on store */
	b	1f
7:	/* found, PP=1x */
	mfspr	%r3,SPR_DMISS		/* get the miss address */
	mfsrin	%r1,%r3			/* get the segment register */
	mfsrr1	%r3
	rlwinm	%r3,%r3,18,31,31	/* get PR-bit */
	rlwnm.	%r1,%r1,%r3,1,1		/* get the key */
	bne-	9b			/* protection violation */
8:	/* found, set reference/change bits */
	ldreg	%r1,4(%r2)		/* reload tlb entry */
	ori	%r1,%r1,(PTE_REF|PTE_CHG)
	sth	%r1,6(%r2)
	b	5b
5:	/* not found anywhere */
	mfsrr1	%r3
	lis	%r1,(DSISR_NOTFOUND|DSISR_STORE)@h
					/* set dsisr<1> to flag pte not found */
					/* dsisr<6> to flag store */
1:
	mtctr	%r0			/* restore counter */
	andi.	%r2,%r3,0xffff		/* clean upper srr1 */
	mtsrr1	%r2
	mtdsisr	%r1			/* load the dsisr */
	mfspr	%r1,SPR_DMISS		/* get the miss address */
	mtdar	%r1			/* put in dar */
	mfmsr	%r0
	xoris	%r0,%r0,PSL_TGPR@h	/* flip the msr<tgpr> bit */
	mtcrf	0x80,%r3		/* restore cr0 */
	mtmsr	%r0			/* now with native gprs */
	isync
#if defined(PPC_HIGH_VEC)
	ba	EXC_HIGHVEC+EXC_DSI
#else
	ba	EXC_DSI
#endif
_C_LABEL(tlbdsmsize) = .-_C_LABEL(tlbdsmiss)
#endif /* !PPC_OEA64 && !PPC_MPC8XX */

#if defined(DDB) || defined(KGDB)
/*
 * In case of DDB we want a separate trap catcher for it
 */
	.local	ddbstk
	.comm	ddbstk,INTSTK,8		/* ddb stack */

/* LINTSTUB: Var: int ddblow[1], ddbsize[1]; */
	.globl	_C_LABEL(ddblow),_C_LABEL(ddbsize)
_C_LABEL(ddblow):
	mtsprg1	%r1			/* save SP */
	ENABLE_64BIT_BRIDGE(%r1)
	mtsprg2 %r29			/* save r29 */
	mfcr	%r29			/* save CR in r29 */
	mfsrr1	%r1
	mtcr	%r1
	GET_CPUINFO(%r1)
	bf	MSR_PR,1f		/* branch if privileged */
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)	/* free r28 */
	mfsprg2	%r28
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)	/* free r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)	/* free r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)	/* free r31 */
	mflr	%r28			/* save LR */
#if defined(DISTANT_KERNEL)
	lis	%r31,u_trap@ha
	addi	%r31,%r31,u_trap@l
	mtlr	%r31
	blrl
#else
	bla	u_trap
#endif
1:
	streg	%r28,(CI_DDBSAVE+CPUSAVE_R28)(%r1) /* save r28 */
	mfsprg2	%r28
	streg	%r28,(CI_DDBSAVE+CPUSAVE_R29)(%r1) /* save r29 */
	streg	%r30,(CI_DDBSAVE+CPUSAVE_R30)(%r1) /* save r30 */
	streg	%r31,(CI_DDBSAVE+CPUSAVE_R31)(%r1) /* save r31 */
	mflr	%r28			/* save LR */
	lis	%r1,ddbstk+INTSTK@ha	/* get new SP */
	addi	%r1,%r1,ddbstk+INTSTK@l
#if defined(DISTANT_KERNEL)
	lis	%r31,ddbtrap@ha
	addi	%r31,%r31,ddbtrap@l
	mtlr	%r31
	blrl
#else
	bla	ddbtrap
#endif
_C_LABEL(ddbsize) = .-_C_LABEL(ddblow)
#endif	/* DDB || KGDB */

/*
 * FRAME_SETUP assumes:
 *	SPRG1		SP (%r1)
 *	savearea	r28-r31,DAR,DSISR	(DAR & DSISR only for DSI traps)
 *	28		LR
 *	29		CR
 *	30		scratch
 *	31		scratch
 *	1		kernel stack
 *	LR		trap type
 *	SRR0/1		as at start of trap
 */
#define	FRAME_SETUP(savearea)						\
/* Have to enable translation to allow access of kernel stack: */	\
	GET_CPUINFO(%r31);						\
	mfsrr0	%r30;							\
	streg	%r30,(savearea+CPUSAVE_SRR0)(%r31);	/* save SRR0 */	\
	mfsrr1	%r30;							\
	streg	%r30,(savearea+CPUSAVE_SRR1)(%r31);	/* save SRR1 */	\
	mfmsr	%r30;							\
	ori	%r30,%r30,(PSL_DR|PSL_IR);	/* turn on relocation */ \
	mtmsr	%r30;			/* stack can be accessed now */	\
	isync;								\
	mfsprg1	%r31;			/* get saved SP */		\
	stregu	%r31,-FRAMELEN(%r1);	/* save it in the callframe */	\
	streg	%r0,FRAME_R0(%r1);	/* save R0 in the trapframe */ \
	streg	%r31,FRAME_R1(%r1);	/* save SP in the trapframe */ \
	streg	%r2,FRAME_R2(%r1);	/* save R2 in the trapframe */ \
	streg	%r28,FRAME_LR(%r1);				\
	stint	%r29,FRAME_CR(%r1);				\
	GET_CPUINFO(%r2);						\
	ldreg	%r31,(savearea+CPUSAVE_R31)(%r2); /* get saved r31 */	\
	ldreg	%r30,(savearea+CPUSAVE_R30)(%r2); /* get saved r30 */	\
	ldreg	%r29,(savearea+CPUSAVE_R29)(%r2); /* get saved r29 */	\
	ldreg	%r28,(savearea+CPUSAVE_R28)(%r2); /* get saved r28 */	\
	streg	%r3,FRAME_R3(%r1);	/* save r3 */		\
	streg	%r4,FRAME_R4(%r1);	/* save r4 */		\
	streg	%r5,FRAME_R5(%r1);	/* save r5 */		\
	streg	%r6,FRAME_R6(%r1);	/* save r6 */		\
	streg	%r7,FRAME_R7(%r1);	/* save r7 */		\
	streg	%r8,FRAME_R8(%r1);	/* save r8 */		\
	streg	%r9,FRAME_R9(%r1);	/* save r9 */		\
	streg	%r10,FRAME_R10(%r1);	/* save r10 */		\
	streg	%r11,FRAME_R11(%r1);	/* save r11 */		\
	streg	%r12,FRAME_R12(%r1);	/* save r12 */		\
	streg	%r13,FRAME_R13(%r1);	/* save r13 */		\
	streg	%r14,FRAME_R14(%r1);	/* save r14 */		\
	streg	%r15,FRAME_R15(%r1);	/* save r15 */		\
	streg	%r16,FRAME_R16(%r1);	/* save r16 */		\
	streg	%r17,FRAME_R17(%r1);	/* save r17 */		\
	streg	%r18,FRAME_R18(%r1);	/* save r18 */		\
	streg	%r19,FRAME_R19(%r1);	/* save r19 */		\
	streg	%r20,FRAME_R20(%r1);	/* save r20 */		\
	streg	%r21,FRAME_R21(%r1);	/* save r21 */		\
	streg	%r22,FRAME_R22(%r1);	/* save r22 */		\
	streg	%r23,FRAME_R23(%r1);	/* save r23 */		\
	streg	%r24,FRAME_R24(%r1);	/* save r24 */		\
	streg	%r25,FRAME_R25(%r1);	/* save r25 */		\
	streg	%r26,FRAME_R26(%r1);	/* save r26 */		\
	streg	%r27,FRAME_R27(%r1);	/* save r27 */		\
	streg	%r28,FRAME_R28(%r1);	/* save r28 */		\
	streg	%r29,FRAME_R29(%r1);	/* save r29 */		\
	streg	%r30,FRAME_R30(%r1);	/* save r30 */		\
	streg	%r31,FRAME_R31(%r1);	/* save r31 */		\
	ldreg	%r28,(savearea+CPUSAVE_DAR)(%r2); /* get saved DAR */	\
	ldreg	%r29,(savearea+CPUSAVE_DSISR)(%r2); /* get saved DSISR */ \
	ldreg	%r30,(savearea+CPUSAVE_SRR0)(%r2); /* get saved SRR0 */	\
	ldreg	%r31,(savearea+CPUSAVE_SRR1)(%r2); /* get saved SRR1 */	\
	ldptr	%r13,CI_CURLWP(%r2);	/* get curlwp */	\
	mfxer	%r3;						\
	mfctr	%r4;						\
	mflr	%r5;						\
	andi.	%r5,%r5,0xff00;					\
	stint	%r3,FRAME_XER(%r1);				\
	streg	%r4,FRAME_CTR(%r1);				\
	streg	%r30,FRAME_SRR0(%r1);			\
	streg	%r31,FRAME_SRR1(%r1);			\
	streg	%r28,FRAME_DAR(%r1);			\
	stint	%r29,FRAME_DSISR(%r1);			\
	stint	%r5,FRAME_EXC(%r1);			\
	SAVE_VRSAVE(%r1,%r6);				\
	SAVE_MQ(%r1,%r7)

#define	FRAME_RESTORE_CALLEE					\
	ldreg	%r31,FRAME_R31(%r1);	/* restore r31 */	\
	ldreg	%r30,FRAME_R30(%r1);	/* restore r30 */	\
	ldreg	%r29,FRAME_R29(%r1);	/* restore r29 */	\
	ldreg	%r28,FRAME_R28(%r1);	/* restore r28 */	\
	ldreg	%r27,FRAME_R27(%r1);	/* restore r27 */	\
	ldreg	%r26,FRAME_R26(%r1);	/* restore r26 */	\
	ldreg	%r25,FRAME_R25(%r1);	/* restore r25 */	\
	ldreg	%r24,FRAME_R24(%r1);	/* restore r24 */	\
	ldreg	%r23,FRAME_R23(%r1);	/* restore r23 */	\
	ldreg	%r22,FRAME_R22(%r1);	/* restore r22 */	\
	ldreg	%r21,FRAME_R21(%r1);	/* restore r21 */	\
	ldreg	%r20,FRAME_R20(%r1);	/* restore r20 */	\
	ldreg	%r19,FRAME_R19(%r1);	/* restore r19 */	\
	ldreg	%r18,FRAME_R18(%r1);	/* restore r18 */	\
	ldreg	%r17,FRAME_R17(%r1);	/* restore r17 */	\
	ldreg	%r16,FRAME_R16(%r1);	/* restore r16 */	\
	ldreg	%r15,FRAME_R15(%r1);	/* restore r15 */	\
	ldreg	%r14,FRAME_R14(%r1);	/* restore r14 */

#define	FRAME_LEAVE(savearea)					\
/* Now restore regs: */						\
	ldreg	%r2,FRAME_SRR0(%r1);				\
	ldreg	%r3,FRAME_SRR1(%r1);				\
	ldreg	%r4,FRAME_CTR(%r1);				\
	ldint	%r5,FRAME_XER(%r1);				\
	ldreg	%r6,FRAME_LR(%r1);				\
	RESTORE_MQ(%r1,%r8);						\
	RESTORE_VRSAVE(%r1,%r9);					\
	GET_CPUINFO(%r7);						\
	streg	%r2,(savearea+CPUSAVE_SRR0)(%r7); /* save SRR0 */	\
	streg	%r3,(savearea+CPUSAVE_SRR1)(%r7); /* save SRR1 */	\
	ldint	%r7,FRAME_CR(%r1);				\
	mtctr	%r4;							\
	mtxer	%r5;							\
	mtlr	%r6;							\
	mtsprg1	%r7;			/* save cr */			\
	ldreg	%r13,FRAME_R13(%r1);	/* restore r13 */	\
	ldreg	%r12,FRAME_R12(%r1);	/* restore r12 */	\
	ldreg	%r11,FRAME_R11(%r1);	/* restore r11 */	\
	ldreg	%r10,FRAME_R10(%r1);	/* restore r10 */	\
	ldreg	%r9,FRAME_R9(%r1);	/* restore r9 */	\
	ldreg	%r8,FRAME_R8(%r1);	/* restore r8 */	\
	ldreg	%r7,FRAME_R7(%r1);	/* restore r7 */	\
	ldreg	%r6,FRAME_R6(%r1);	/* restore r6 */	\
	ldreg	%r5,FRAME_R5(%r1);	/* restore r5 */	\
	ldreg	%r4,FRAME_R4(%r1);	/* restore r4 */	\
	ldreg	%r3,FRAME_R3(%r1);	/* restore r3 */	\
	ldreg	%r2,FRAME_R2(%r1);	/* restore r2 */	\
	ldreg	%r0,FRAME_R0(%r1);	/* restore r0 */	\
	ldreg	%r1,FRAME_R1(%r1);	/* restore old sp in r1 */ \
/* Can't touch %r1 from here on */					\
	mtsprg2	%r2;			/* save r2 & r3 */		\
	mtsprg3	%r3;							\
/* Disable translation, machine check and recoverability: */		\
	mfmsr	%r2;							\
	andi.	%r2,%r2,~(PSL_DR|PSL_IR|PSL_ME|PSL_RI)@l;		\
	mtmsr	%r2;							\
	isync;								\
/* Decide whether we return to user mode: */				\
	GET_CPUINFO(%r2);						\
	ldreg	%r3,(savearea+CPUSAVE_SRR1)(%r2);			\
	mtcr	%r3;							\
	bf	MSR_PR,1f;		/* branch if PSL_PR is false */	\
/* Restore user SRs */							\
	RESTORE_USER_SRS(%r2,%r3);					\
1:	mfsprg1	%r2;			/* restore cr */		\
	mtcr	%r2;							\
	GET_CPUINFO(%r2);						\
	ldreg	%r3,(savearea+CPUSAVE_SRR0)(%r2);			\
	mtsrr0	%r3;							\
	ldreg	%r3,(savearea+CPUSAVE_SRR1)(%r2);			\
	mtsrr1	%r3;							\
	mfsprg2	%r2;			/* restore r2 & r3 */		\
	mfsprg3	%r3

/*
 * Preamble code for DSI/ISI traps
 */
disitrap:
	GET_CPUINFO(%r1)
	ldreg	%r30,(CI_DISISAVE+CPUSAVE_R28)(%r1)
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)
	ldreg	%r31,(CI_DISISAVE+CPUSAVE_R29)(%r1)
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)
	ldreg	%r30,(CI_DISISAVE+CPUSAVE_R30)(%r1)
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)
	ldreg	%r31,(CI_DISISAVE+CPUSAVE_R31)(%r1)
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)
	mfdar	%r30
	mfdsisr	%r31
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_DAR)(%r1)
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_DSISR)(%r1)

#ifdef DDB
	mfsrr1	%r31
	mtcr	%r31
	bt	MSR_PR,trapstart	/* branch is user mode */
	mfsprg1	%r31			/* get old SP */
#if 0
	subf	%r30,%r30,%r31		/* subtract DAR from it */
	addi	%r30,%r30,2048		/* offset result 1/2 page */
	cmplwi	%cr0,%r30,4096		/* is DAR +- 1/2 page of SP? */
#else
	xor.	%r30,%r30,%r31		/* try xor most significant bits */
	cmplwi	%cr0,%r30,4096		/* is DAR on same page as SP? */
#endif
	bge	%cr0,trapstart		/* no, too far away. */
	/* Now convert this DSI into a DDB trap.  */
	GET_CPUINFO(%r1)
	ldreg	%r30,(CI_DISISAVE+CPUSAVE_R28)(%r1) /* get  r28 */
	streg	%r30,(CI_DDBSAVE +CPUSAVE_R28)(%r1) /* save r28 */
	ldreg	%r31,(CI_DISISAVE+CPUSAVE_R29)(%r1) /* get  r29 */
	streg	%r31,(CI_DDBSAVE +CPUSAVE_R29)(%r1) /* save r29 */
	ldreg	%r30,(CI_DISISAVE+CPUSAVE_R30)(%r1) /* get  r30 */
	streg	%r30,(CI_DDBSAVE +CPUSAVE_R30)(%r1) /* save r30 */
	ldreg	%r31,(CI_DISISAVE+CPUSAVE_R31)(%r1) /* get  r31 */
	streg	%r31,(CI_DDBSAVE +CPUSAVE_R31)(%r1) /* save r31 */
	lis	%r1,ddbstk+INTSTK@ha	/* get new SP */
	addi	%r1,%r1,ddbstk+INTSTK@l
	b	ddbtrap
#endif

	.globl	_C_LABEL(trapstart)
	.type	_C_LABEL(trapstart),@function
_C_LABEL(trapstart):
realtrap:
/* Test whether we already had PR set */
	mfsrr1	%r1
	mtcr	%r1
	mfsprg1	%r1			/* restore SP (might have been
					   overwritten) */
s_trap:
	bf	MSR_PR,k_trap		/* branch if PSL_PR is false */
	GET_CPUINFO(%r1)		/* get cpu_info for this cpu */
u_trap:
	ldptr	%r1,CI_CURPCB(%r1)
	addi	%r1,%r1,USPACE-CALLFRAMELEN /* stack is top of user struct */

/*
 * Now the common trap catching code.
 */

	RESTORE_KERN_SRS(%r30,%r31)	/* First enable KERNEL mapping */

k_trap:
	FRAME_SETUP(CI_TEMPSAVE)
trapagain:
/* Now we can recover interrupts again: */
	mfmsr	%r7
	ldreg	%r6, FRAME_SRR1(%r1)
	andi.	%r6,%r6,(PSL_EE|PSL_ME|PSL_RI)@l
	or	%r7,%r7,%r6
	mtmsr	%r7
	isync
/* Call C trap code: */
	addi	%r3,%r1,FRAME_TF
	bl	_C_LABEL(trap)
/* LINTSTUB: Var: int trapexit[1]; */
	.globl	trapexit
trapexit:
/* Disable interrupts: */
	mfmsr	%r3
	andi.	%r3,%r3,~PSL_EE@l
	mtmsr	%r3
/* Test AST pending: */
	mtcr	%r31
	bf	MSR_PR,trapleave	/* branch if PSL_PR is false */
	ldint	%r4,L_MD_ASTPENDING(%r13)
	andi.	%r4,%r4,1
	beq	trapleave

	li	%r6,EXC_AST
	stint	%r6,FRAME_EXC(%r1)
	b	trapagain

trapleave:
	FRAME_RESTORE_CALLEE
intrleave:
	FRAME_LEAVE(CI_TEMPSAVE)
	RFI

/*
 * Trap handler for syscalls (EXC_SC)
 */
/* LINTSTUB: Var: int sctrap[1], scsize[1]; */
	.globl	_C_LABEL(sctrap),_C_LABEL(scsize),_C_LABEL(sctrapexit)
_C_LABEL(sctrap):
	mtsprg1	%r1			/* save SP */
	ENABLE_64BIT_BRIDGE(%r1)
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1) /* save r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1) /* save r20 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1) /* save r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1) /* save r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
#if defined(DISTANT_KERNEL)
	lis	%r31,s_sctrap@ha
	addi	%r31,%r31,s_sctrap@l
	mtlr	%r31
	blrl
#else
	bla	s_sctrap
#endif
	_C_LABEL(scsize) = .-_C_LABEL(sctrap)

s_sctrap:
	GET_CPUINFO(%r1)
	ldptr	%r1,CI_CURPCB(%r1)
	addi	%r1,%r1,USPACE-CALLFRAMELEN /* stack is top of user struct */
	RESTORE_KERN_SRS(%r30,%r31)	/* First enable KERNEL mapping */
	FRAME_SETUP(CI_TEMPSAVE)
/* Now we can recover interrupts again: */
	mfmsr	%r7
	ori	%r7,%r7,(PSL_EE|PSL_ME|PSL_RI)@l
	mtmsr	%r7
	isync
	addi	%r3,%r1,FRAME_TF
/* Call the appropriate syscall handler: */
	ldptr	%r4,L_PROC(%r13)
	ldptr	%r4,P_MD_SYSCALL(%r4)
	mtctr	%r4
	bctrl
_C_LABEL(sctrapexit):
	b	trapexit

/*
 * External interrupt second level handler
 */
/*
 * INTR_SETUP assumes:
 *	SPRG1		SP (%r1)
 *	savearea	r28-r31
 *	28		LR
 *	29		CR
 *	30		scratch
 *	31		scratch
 *	1		kernel stack
 *	SRR0/1		as at start of exception
 */
#define	INTR_SETUP(savearea,exc)					\
/* Have to enable translation to allow access of kernel stack: */	\
	GET_CPUINFO(%r31);						\
	mfsrr0	%r30;							\
	streg	%r30,(savearea+CPUSAVE_SRR0)(%r31);	/* save SRR0 */	\
	mfsrr1	%r30;							\
	streg	%r30,(savearea+CPUSAVE_SRR1)(%r31);	/* save SRR1 */	\
	mfmsr	%r30;							\
	ori	%r30,%r30,(PSL_DR|PSL_IR);	/* turn on relocation */ \
	mtmsr	%r30;			/* stack can be accessed now */	\
	isync;								\
	mfsprg1	%r31;			/* get saved SP */		\
	stregu	%r31,-FRAMELEN(%r1);	/* save it in the callframe */	\
	streg	%r0,FRAME_R0(%r1);	/* save R0 in the trapframe */ \
	streg	%r31,FRAME_R1(%r1);	/* save SP in the trapframe */ \
	streg	%r2,FRAME_R2(%r1);	/* save R2 in the trapframe */ \
	streg	%r28,FRAME_LR(%r1);				\
	stint	%r29,FRAME_CR(%r1);				\
	GET_CPUINFO(%r2);						\
	ldreg	%r31,(savearea+CPUSAVE_R31)(%r2); /* get saved r31 */	\
	ldreg	%r30,(savearea+CPUSAVE_R30)(%r2); /* get saved r30 */	\
	ldreg	%r29,(savearea+CPUSAVE_R29)(%r2); /* get saved r29 */	\
	ldreg	%r28,(savearea+CPUSAVE_R28)(%r2); /* get saved r28 */	\
	streg	%r3,FRAME_R3(%r1);	/* save r3 */		\
	streg	%r4,FRAME_R4(%r1);	/* save r4 */		\
	streg	%r5,FRAME_R5(%r1);	/* save r5 */		\
	streg	%r6,FRAME_R6(%r1);	/* save r6 */		\
	streg	%r7,FRAME_R7(%r1);	/* save r7 */		\
	streg	%r8,FRAME_R8(%r1);	/* save r8 */		\
	streg	%r9,FRAME_R9(%r1);	/* save r9 */		\
	streg	%r10,FRAME_R10(%r1);	/* save r10 */		\
	streg	%r11,FRAME_R11(%r1);	/* save r11 */		\
	streg	%r12,FRAME_R12(%r1);	/* save r12 */		\
	streg	%r13,FRAME_R13(%r1);	/* save r13 */		\
	ldreg	%r11,(savearea+CPUSAVE_SRR0)(%r2); /* get saved SRR0 */	\
	ldreg	%r12,(savearea+CPUSAVE_SRR1)(%r2); /* get saved SRR1 */	\
	ldptr	%r13,CI_CURLWP(%r2);	/* get curlwp */	\
	ldint	%r3,CI_IDEPTH(%r2);					\
	stint	%r3,FRAME_IDEPTH(%r1);					\
	mfxer	%r3;							\
	mfctr	%r4;							\
	li	%r5,exc;						\
	stint	%r5,FRAME_EXC(%r1);					\
	stint	%r3,FRAME_XER(%r1);					\
	streg	%r4,FRAME_CTR(%r1);					\
	streg	%r11,FRAME_SRR0(%r1);					\
	streg	%r12,FRAME_SRR1(%r1);					\
	mfmsr	%r6;							\
	ori	%r6,%r6,PSL_RI;		/* turn on recovery interrupt */\
	mtmsr	%r6;							\
	SAVE_VRSAVE(%r1,%r6);						\
	SAVE_MQ(%r1,%r7)

/* LINTSTUB: Var: int extint_call[1]; */
/*
 * R1=sp, R28=LR, R29=CR, R30=scratch, R31=scratch
 */
	.globl	_C_LABEL(extint_call)
extintr:
	INTR_SETUP(CI_TEMPSAVE, EXC_EXI)
					/* make trapframe available */
	addi	%r3,%r1,FRAME_TF	/* kern frame -> trap frame */
_C_LABEL(extint_call):
	bl	_C_LABEL(extint_call)	/* to be filled in later */

intr_exit:
/* Disable interrupts (should already be disabled) but not MMU here: */
	mfmsr	%r3
	andi.	%r3,%r3,~(PSL_EE|PSL_ME|PSL_RI)@l
	mtmsr	%r3
	isync

/* Returning to user mode? */
	ldreg	%r4,FRAME_SRR1(%r1)
	mtcr	%r4			/* saved SRR1 */
	bf	MSR_PR,intrleave	/* branch if PSL_PR is false */

	ldint	%r3,L_MD_ASTPENDING(%r13) /* Test AST pending */
	andi.	%r3,%r3,1
	beq	intrleave		/* common frame exit */

/*
 * Since interrupts save their state in a std trapframe, all we need to do to
 * process the AST is finish filling the trapframe with the rest of the fixed
 * registers and let trap deal with it.
 */
	streg	%r14,FRAME_R14(%r1)
	streg	%r15,FRAME_R15(%r1)
	streg	%r16,FRAME_R16(%r1)
	streg	%r17,FRAME_R17(%r1)
	streg	%r18,FRAME_R18(%r1)
	streg	%r19,FRAME_R19(%r1)
	streg	%r20,FRAME_R20(%r1)
	streg	%r21,FRAME_R21(%r1)
	streg	%r22,FRAME_R22(%r1)
	streg	%r23,FRAME_R23(%r1)
	streg	%r24,FRAME_R24(%r1)
	streg	%r25,FRAME_R25(%r1)
	streg	%r26,FRAME_R26(%r1)
	streg	%r27,FRAME_R27(%r1)
	streg	%r28,FRAME_R28(%r1)
	streg	%r29,FRAME_R29(%r1)
	streg	%r30,FRAME_R30(%r1)
	streg	%r31,FRAME_R31(%r1)

	/*
	 * Tell trap we are doing an AST.
	 */
	li	%r6,EXC_AST
	stint	%r6,FRAME_EXC(%r1)

	mr	%r31, %r4		/* trapagain wants SRR1 in %r31 */
	b	trapagain

/*
 * Decrementer interrupt second level handler
 */
decrintr:
	INTR_SETUP(CI_TEMPSAVE, EXC_DECR)

	addi	%r3,%r1,FRAME_CF	/* intr frame -> clock frame */
	bl	_C_LABEL(decr_intr)
	b	intr_exit

#ifdef DDB
/*
 * Deliberate entry to ddbtrap
 */
	.globl	_C_LABEL(ddb_trap)
_C_LABEL(ddb_trap):
	mtsprg1	%r1
	mfmsr	%r3
	mtsrr1	%r3
	andi.	%r3,%r3,~(PSL_EE|PSL_ME)@l
	mtmsr	%r3			/* disable interrupts */
	isync
	ENABLE_64BIT_BRIDGE(%r3)
	GET_CPUINFO(%r3)
	streg	%r28,(CI_DDBSAVE+CPUSAVE_R28)(%r3)
	streg	%r29,(CI_DDBSAVE+CPUSAVE_R29)(%r3)
	streg	%r30,(CI_DDBSAVE+CPUSAVE_R30)(%r3)
	streg	%r31,(CI_DDBSAVE+CPUSAVE_R31)(%r3)
	mflr	%r28
	li	%r29,EXC_BPT
	mtlr	%r29
	mfcr	%r29
	mtsrr0	%r28
#endif /* DDB */

#if defined(DDB) || defined(KGDB)
/*
 * Now the ddb trap catching code.
 */
ddbtrap:
	FRAME_SETUP(CI_DDBSAVE)
/* Call C trap code: */
	addi	%r3,%r1,FRAME_TF
	bl	_C_LABEL(ddb_trap_glue)
	or.	%r3,%r3,%r3
	beq	trapagain
	FRAME_RESTORE_CALLEE
	FRAME_LEAVE(CI_DDBSAVE)
	RFI
#endif /* DDB || KGDB */

	.globl	_C_LABEL(trapend)
_C_LABEL(trapend):

/*
 * All OEA have FPUs so include this too.  Some OEA have AltiVec so include
 * that too.
 */
#if !defined(PPC_MPC8XX)
#include <powerpc/powerpc/fpu_subr.S>
#include <powerpc/oea/altivec_subr.S>
#endif
