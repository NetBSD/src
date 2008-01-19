/*	$NetBSD: trap_subr.S,v 1.59.48.1 2008/01/19 12:14:43 bouyer Exp $	*/

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
#include "opt_altivec.h"

/* LINTSTUB: include <sys/param.h> */
/* LINTSTUB: include <powerpc/oea/bat.h> */

#ifdef ALTIVEC
#define	SAVE_VRSAVE(tf,b)						\
	mfspr	b,SPR_VRSAVE;						\
	stint	b,(FRAME_VRSAVE+(2*SZREG))(tf);

#define RESTORE_VRSAVE(tf,b)						\
	ldint	b,(FRAME_VRSAVE+(2*SZREG))(tf);				\
	mtspr	SPR_VRSAVE,b;
#else
#define SAVE_VRSAVE(tf,b)
#define RESTORE_VRSAVE(tf,b)
#endif

#if defined (PPC_OEA)
#define	RFI	rfi
#elif defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#define	RFI	rfid
#endif /* PPC_OEA */

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

#elif defined(PPC_OEA)

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
#elif defined (PPC_OEA64_BRIDGE)
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
	RESTORE_SRS(pmap,sr);
/*
 * Kernel SRs are loaded directly from kernel_pmap_.
 * Note: oea_init() relies on the 601 instruction sequence.
 */
#define RESTORE_KERN_SRS(pmap,sr)					\
	lis	pmap,_C_LABEL(kernel_pmap_)@ha;				\
	ldregu	sr,_C_LABEL(kernel_pmap_)+PM_SR@l(pmap);		\
	RESTORE_SRS(pmap,sr);

#endif /* (PPC_OEA64) */

/*
 * Save/restore MPC601 MQ register.
 * Note: oea_init() relies on this instruction sequence.
 */
#if !defined(PPC_OEA64) && !defined(PPC_OEA64_BRIDGE)
#define	SAVE_MQ(tf,b)							\
	mfspr	b,SPR_MQ;						\
	streg	b,(FRAME_MQ+(2*SZREG))(tf);

#define	RESTORE_MQ(tf,b)						\
	ldreg	b,(FRAME_MQ+(2*SZREG))(tf);				\
	mtspr	SPR_MQ,b;
#else
#define	SAVE_MQ(tf,b)
#define	RESTORE_MQ(tf,b)
#endif

/*
 * This code gets copied to all the trap vectors
 * (except ISI/DSI, ALI, the interrupts, and possibly the debugging
 * traps when using IPKDB).
 */
/* LINTSTUB: Var: int trapcode[1], trapsize[1]; */
	.text
	.globl	_C_LABEL(trapcode),_C_LABEL(trapsize)
_C_LABEL(trapcode):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)	/* free r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)	/* free r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)	/* free r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)	/* free r31 */
#if defined (PPC_OEA64_BRIDGE)
	mfmsr	%r28
	clrldi  %r28,%r28,1
	mtmsrd	%r28
	isync
#endif
	mfsprg1	%r1			/* restore SP */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
/* Test whether we already had PR set */
	mfsrr1	%r31
	mtcr	%r31
	bla	s_trap
_C_LABEL(trapsize) = .-_C_LABEL(trapcode)

/*
 * For ALI: has to save DSISR and DAR
 */
/* LINTSTUB: Var: int alicode[1], alisize[1]; */
	.globl	_C_LABEL(alitrap),_C_LABEL(alisize)
_C_LABEL(alitrap):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1) /* save r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1) /* save r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1) /* save r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1) /* save r31 */
#if defined (PPC_OEA64_BRIDGE)
	mfmsr	%r28
	clrldi  %r28,%r28,1
	mtmsrd	%r28
	isync
#endif
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
	bla	s_trap
_C_LABEL(alisize) = .-_C_LABEL(alitrap)

/*
 * Similar to the above for DSI
 * Has to handle BAT spills
 * and standard pagetable spills
 */
/* LINTSTUB: Var: int dsicode[1], dsisize[1]; */
	.globl	_C_LABEL(dsitrap),_C_LABEL(dsisize)
_C_LABEL(dsitrap):
	mtsprg1	%r1
	GET_CPUINFO(%r1)
	streg	%r28,(CI_DISISAVE+CPUSAVE_R28)(%r1)	/* save r28 */
	streg	%r29,(CI_DISISAVE+CPUSAVE_R29)(%r1)	/* save r29 */
	streg	%r30,(CI_DISISAVE+CPUSAVE_R30)(%r1)	/* save r30 */
	streg	%r31,(CI_DISISAVE+CPUSAVE_R31)(%r1)	/* save r31 */
#if defined (PPC_OEA64_BRIDGE)
	mfmsr	%r28
	clrldi  %r28,%r28,1
	mtmsrd	%r28
	isync
#endif
	mfsprg1	%r1
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	mtsprg2	%r30			/* in SPRG2 */
	mfsrr1	%r31			/* test kernel mode */
	mtcr	%r31
	bt	17,1f			/* branch if PSL_PR is set */
/* XXXSL: for the PPC64 bridge, we have skipped over the initialization
 * of battable, hence the supervisor valid bit should not be set
 * and we should end up following the code path into the page fault handler
 */
#if defined (PPC_OEA) || defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
	mfdar	%r31			/* get fault address */
	rlwinm	%r31,%r31,7,25,28	/* get segment * 8 */

	/* get batu */
	addis	%r31,%r31,_C_LABEL(battable)@ha
	ldreg	%r30,_C_LABEL(battable)@l(%r31)
	mtcr	%r30
	bf	30,1f			/* branch if supervisor valid is
					   false */
	/* get batl */
	ldreg	%r31,_C_LABEL(battable)+SZREG@l(%r31)
/* We randomly use the highest two bat registers here */
	mftb	%r28
	andi.	%r28,%r28,1
	bne	2f
	mtdbatu	2,%r30
	mtdbatl	2,%r31
	b	3f
2:
	mtdbatu	3,%r30
	mtdbatl	3,%r31
3:
#endif /* PPC_OEA || PPC_OEA64  || PPC_OEA64_BRIDGE */

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
	mtsprg1	%r1			/* save SP */
	bla	disitrap
_C_LABEL(dsisize) = .-_C_LABEL(dsitrap)

#if !defined(PPC_OEA64)
/*
 * Dedicated MPC601 version of the above.
 * Considers different BAT format and combined implementation
 * (being addressed as I-BAT).
 */
/* LINTSTUB: Var: int dsi601code[1], dsi601size[1]; */
	.globl	_C_LABEL(dsi601trap),_C_LABEL(dsi601size)
_C_LABEL(dsi601trap):
	mtsprg1	%r1
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
	bt	17,1f			/* branch if PSL_PR is set */
#if defined (PPC_OEA) && !defined(PPC_OEA64) && !defined (PPC_OEA64_BRIDGE)
	mfdar	%r31			/* get fault address */
	rlwinm	%r31,%r31,12,20,28	/* get "segment" battable offset */

	/* get batl */
	addis	%r31,%r31,_C_LABEL(battable)@ha
	ldreg	%r30,_C_LABEL(battable)+SZREG@l(%r31)
	mtcr	%r30
	bf	25,1f			/* branch if Valid is is false,
					   presently assumes supervisor only */

	/* get batu */
	ldreg	%r31,_C_LABEL(battable)@l(%r31)
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
#endif /* PPC_OEA && !PPC_OEA64  && !PPC_OEA64_BRIDGE */

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
	bla	disitrap
_C_LABEL(dsi601size) = .-_C_LABEL(dsi601trap)
#endif /* !defined(PPC_OEA64) */

/*
 * This one for the external interrupt handler.
 */
/* LINTSTUB: Var: int extint[1], extsize[1]; */
	.globl	_C_LABEL(extint),_C_LABEL(extsize)
_C_LABEL(extint):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)	/* save r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)	/* save r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)	/* save r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)	/* save r31 */
#if defined (PPC_OEA64_BRIDGE)
	mfmsr	%r28
	clrldi  %r28,%r28,1
	mtmsrd	%r28
	isync
#endif
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	ldint	%r31,CI_INTRDEPTH(%r1)	/* were we already running on intstk? */
	addic.	%r31,%r31,1
	stint	%r31,CI_INTRDEPTH(%r1)
	ldptr	%r1,CI_INTSTK(%r1)	/* get interrupt stack */
	beq	1f
	mfsprg1	%r1			/* yes, get old SP */
1:
	ba	extintr
_C_LABEL(extsize) = .-_C_LABEL(extint)

/*
 * And this one for the decrementer interrupt handler.
 */
/* LINTSTUB: Var: int decrint[1], decrsize[1]; */
	.globl	_C_LABEL(decrint),_C_LABEL(decrsize)
_C_LABEL(decrint):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)	/* save r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)	/* save r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)	/* save r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)	/* save r31 */
#if defined (PPC_OEA64_BRIDGE)
	mfmsr	%r28
	clrldi  %r28,%r28,1
	mtmsrd	%r28
	isync
#endif
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	ldint	%r31,CI_INTRDEPTH(%r1)	/* were we already running on intstk? */
	addic.	%r31,%r31,1
	stint	%r31,CI_INTRDEPTH(%r1)
	ldptr	%r1,CI_INTSTK(%r1)	/* get interrupt stack */
	beq	1f
	mfsprg1	%r1			/* yes, get old SP */
1:
	ba	decrintr
_C_LABEL(decrsize) = .-_C_LABEL(decrint)

#if !defined(PPC_OEA64)
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
	cmpl	%cr0,%r1,%r3		/* see if found pte */
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
	ba	EXC_ISI
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
	cmpl	%cr0,%r1,%r3		/* see if found pte */
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
	ba	EXC_DSI
_C_LABEL(tlbdlmsize) = .-_C_LABEL(tlbdlmiss)

/* LINTSTUB: Var: int tlbdsmiss[1], tlbdsmsize[1]; */
	.globl	_C_LABEL(tlbdsmiss),_C_LABEL(tlbdsmsize)
_C_LABEL(tlbdsmiss):
	mfspr	%r2,SPR_HASH1		/* get first pointer */
	li	%r1,%r8
	mfctr	%r0			/* save counter */
	mfspr	%r3,SPR_DCMP		/* get first compare value */
	addi	%r2,%r2,-8		/* predec pointer */
1:
	mtctr	%r1			/* load counter */
2:
	ldregu	%r1,8(%r2)		/* get next pte */
	cmpl	%cr0,%r1,%r3		/* see if found pte */
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
	rlwnm.	%r2,%r2,%r3,1,1		/* get the key */
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
	ba	EXC_DSI
_C_LABEL(tlbdsmsize) = .-_C_LABEL(tlbdsmiss)
#endif /* !defined(PPC_OEA64) */

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
	mtsprg2 %r29			/* save r29 */
#if defined (PPC_OEA64_BRIDGE)
	mfmsr	%r29
	clrldi  %r29,%r29,1
	mtmsrd	%r29
	isync
#endif
	mfcr	%r29			/* save CR in r29 */
	mfsrr1	%r1
	mtcr	%r1
	GET_CPUINFO(%r1)
	bf	17,1f			/* branch if privileged */
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)	/* free r28 */
	mfsprg2	%r28
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)	/* free r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)	/* free r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)	/* free r31 */
	mflr	%r28			/* save LR */
	bla	u_trap
1:
	streg	%r28,(CI_DDBSAVE+CPUSAVE_R28)(%r1) /* save r28 */
	mfsprg2	%r28
	streg	%r28,(CI_DDBSAVE+CPUSAVE_R29)(%r1) /* save r29 */
	streg	%r30,(CI_DDBSAVE+CPUSAVE_R30)(%r1) /* save r30 */
	streg	%r31,(CI_DDBSAVE+CPUSAVE_R31)(%r1) /* save r31 */
	mflr	%r28			/* save LR */
	lis	%r1,ddbstk+INTSTK@ha	/* get new SP */
	addi	%r1,%r1,ddbstk+INTSTK@l
	bla	ddbtrap
_C_LABEL(ddbsize) = .-_C_LABEL(ddblow)
#endif	/* DDB || KGDB */

#ifdef IPKDB
#define	ipkdbsave	0xde0		/* primary save area for IPKDB */
/*
 * In case of IPKDB we want a separate trap catcher for it
 */

	.local	ipkdbstk
	.comm	ipkdbstk,INTSTK,8		/* ipkdb stack */

/* LINTSTUB: Var: int ipkdblow[1], ipkdbsize[1]; */
	.globl	_C_LABEL(ipkdblow),_C_LABEL(ipkdbsize)
_C_LABEL(ipkdblow):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	streg	%r28,(CI_IPKDBSAVE+CPUSAVE_R28)(%r1) /* save r28 */
	streg	%r29,(CI_IPKDBSAVE+CPUSAVE_R29)(%r1) /* save r29 */
	streg	%r30,(CI_IPKDBSAVE+CPUSAVE_R30)(%r1) /* save r30 */
	streg	%r31,(CI_IPKDBSAVE+CPUSAVE_R31)(%r1) /* save r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	lis	%r1,ipkdbstk+INTSTK@ha	/* get new SP */
	addi	%r1,%r1,ipkdbstk+INTSTK@l
	bla	ipkdbtrap
_C_LABEL(ipkdbsize) = .-_C_LABEL(ipkdblow)
#endif	/* IPKDB */

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
	mtmsr	%r30;			/* stack can be accesed now */	\
	isync;								\
	mfsprg1	%r31;			/* get saved SP */		\
	stregu	%r31,-FRAMELEN(%r1);	/* save it in the callframe */	\
	streg	%r0,(FRAME_0+(2*SZREG))(%r1);	/* save R0 in the trapframe */ \
	streg	%r31,(FRAME_1+(2*SZREG))(%r1);	/* save SP in the trapframe */ \
	streg	%r2,(FRAME_2+(2*SZREG))(%r1);	/* save R2 in the trapframe */ \
	streg	%r28,(FRAME_LR+(2*SZREG))(%r1);				\
	stint	%r29,(FRAME_CR+(2*SZREG))(%r1);				\
	GET_CPUINFO(%r2);						\
	ldreg	%r31,(savearea+CPUSAVE_R31)(%r2); /* get saved r31 */	\
	ldreg	%r30,(savearea+CPUSAVE_R30)(%r2); /* get saved r30 */	\
	ldreg	%r29,(savearea+CPUSAVE_R29)(%r2); /* get saved r29 */	\
	ldreg	%r28,(savearea+CPUSAVE_R28)(%r2); /* get saved r28 */	\
	streg	%r3,(FRAME_3+(2*SZREG))(%r1);	/* save r3 */		\
	streg	%r4,(FRAME_4+(2*SZREG))(%r1);	/* save r4 */		\
	streg	%r5,(FRAME_5+(2*SZREG))(%r1);	/* save r5 */		\
	streg	%r6,(FRAME_6+(2*SZREG))(%r1);	/* save r6 */		\
	streg	%r7,(FRAME_7+(2*SZREG))(%r1);	/* save r7 */		\
	streg	%r8,(FRAME_8+(2*SZREG))(%r1);	/* save r8 */		\
	streg	%r9,(FRAME_9+(2*SZREG))(%r1);	/* save r9 */		\
	streg	%r10,(FRAME_10+(2*SZREG))(%r1);	/* save r10 */		\
	streg	%r11,(FRAME_11+(2*SZREG))(%r1);	/* save r11 */		\
	streg	%r12,(FRAME_12+(2*SZREG))(%r1);	/* save r12 */		\
	streg	%r13,(FRAME_13+(2*SZREG))(%r1);	/* save r13 */		\
	streg	%r14,(FRAME_14+(2*SZREG))(%r1);	/* save r14 */		\
	streg	%r15,(FRAME_15+(2*SZREG))(%r1);	/* save r15 */		\
	streg	%r16,(FRAME_16+(2*SZREG))(%r1);	/* save r16 */		\
	streg	%r17,(FRAME_17+(2*SZREG))(%r1);	/* save r17 */		\
	streg	%r18,(FRAME_18+(2*SZREG))(%r1);	/* save r18 */		\
	streg	%r19,(FRAME_19+(2*SZREG))(%r1);	/* save r19 */		\
	streg	%r20,(FRAME_20+(2*SZREG))(%r1);	/* save r20 */		\
	streg	%r21,(FRAME_21+(2*SZREG))(%r1);	/* save r21 */		\
	streg	%r22,(FRAME_22+(2*SZREG))(%r1);	/* save r22 */		\
	streg	%r23,(FRAME_23+(2*SZREG))(%r1);	/* save r23 */		\
	streg	%r24,(FRAME_24+(2*SZREG))(%r1);	/* save r24 */		\
	streg	%r25,(FRAME_25+(2*SZREG))(%r1);	/* save r25 */		\
	streg	%r26,(FRAME_26+(2*SZREG))(%r1);	/* save r26 */		\
	streg	%r27,(FRAME_27+(2*SZREG))(%r1);	/* save r27 */		\
	streg	%r28,(FRAME_28+(2*SZREG))(%r1);	/* save r28 */		\
	streg	%r29,(FRAME_29+(2*SZREG))(%r1);	/* save r29 */		\
	streg	%r30,(FRAME_30+(2*SZREG))(%r1);	/* save r30 */		\
	streg	%r31,(FRAME_31+(2*SZREG))(%r1);	/* save r31 */		\
	ldreg	%r28,(savearea+CPUSAVE_DAR)(%r2); /* get saved DAR */	\
	ldreg	%r29,(savearea+CPUSAVE_DSISR)(%r2); /* get saved DSISR */ \
	ldreg	%r30,(savearea+CPUSAVE_SRR0)(%r2); /* get saved SRR0 */	\
	ldreg	%r31,(savearea+CPUSAVE_SRR1)(%r2); /* get saved SRR1 */	\
	mfxer	%r3;							\
	mfctr	%r4;							\
	mflr	%r5;							\
	andi.	%r5,%r5,0xff00;						\
	stint	%r3,(FRAME_XER+(2*SZREG))(%r1);				\
	streg	%r4,(FRAME_CTR+(2*SZREG))(%r1);				\
	streg	%r30,(FRAME_SRR0+(2*SZREG))(%r1);			\
	streg	%r31,(FRAME_SRR1+(2*SZREG))(%r1);			\
	streg	%r28,(FRAME_DAR+(2*SZREG))(%r1);			\
	stint	%r29,(FRAME_DSISR+(2*SZREG))(%r1);			\
	stint	%r5,(FRAME_EXC+(2*SZREG))(%r1);				\
	SAVE_VRSAVE(%r1,%r6);						\
	SAVE_MQ(%r1,%r7)

#if defined (PPC_OEA64_BRIDGE)
#define	FRAME_LEAVE(savearea)						\
/* Now restore regs: */							\
	ldreg	%r2,(FRAME_SRR0+(2*SZREG))(%r1);			\
	ldreg	%r3,(FRAME_SRR1+(2*SZREG))(%r1);			\
	ldreg	%r4,(FRAME_CTR+(2*SZREG))(%r1);				\
	ldint	%r5,(FRAME_XER+(2*SZREG))(%r1);				\
	ldreg	%r6,(FRAME_LR+(2*SZREG))(%r1);				\
	RESTORE_MQ(%r1,%r8);						\
	RESTORE_VRSAVE(%r1,%r9);					\
	GET_CPUINFO(%r7);						\
	streg	%r2,(savearea+CPUSAVE_SRR0)(%r7); /* save SRR0 */	\
	streg	%r3,(savearea+CPUSAVE_SRR1)(%r7); /* save SRR1 */	\
	ldint	%r7,(FRAME_CR+(2*SZREG))(%r1);				\
	mtctr	%r4;							\
	mtxer	%r5;							\
	mtlr	%r6;							\
	mtsprg1	%r7;			/* save cr */			\
	ldreg	%r31,(FRAME_31+(2*SZREG))(%r1);	/* restore r31 */	\
	ldreg	%r30,(FRAME_30+(2*SZREG))(%r1);	/* restore r30 */	\
	ldreg	%r29,(FRAME_29+(2*SZREG))(%r1);	/* restore r29 */	\
	ldreg	%r28,(FRAME_28+(2*SZREG))(%r1);	/* restore r28 */	\
	ldreg	%r27,(FRAME_27+(2*SZREG))(%r1);	/* restore r27 */	\
	ldreg	%r26,(FRAME_26+(2*SZREG))(%r1);	/* restore r26 */	\
	ldreg	%r25,(FRAME_25+(2*SZREG))(%r1);	/* restore r25 */	\
	ldreg	%r24,(FRAME_24+(2*SZREG))(%r1);	/* restore r24 */	\
	ldreg	%r23,(FRAME_23+(2*SZREG))(%r1);	/* restore r23 */	\
	ldreg	%r22,(FRAME_22+(2*SZREG))(%r1);	/* restore r22 */	\
	ldreg	%r21,(FRAME_21+(2*SZREG))(%r1);	/* restore r21 */	\
	ldreg	%r20,(FRAME_20+(2*SZREG))(%r1);	/* restore r20 */	\
	ldreg	%r19,(FRAME_19+(2*SZREG))(%r1);	/* restore r19 */	\
	ldreg	%r18,(FRAME_18+(2*SZREG))(%r1);	/* restore r18 */	\
	ldreg	%r17,(FRAME_17+(2*SZREG))(%r1);	/* restore r17 */	\
	ldreg	%r16,(FRAME_16+(2*SZREG))(%r1);	/* restore r16 */	\
	ldreg	%r15,(FRAME_15+(2*SZREG))(%r1);	/* restore r15 */	\
	ldreg	%r14,(FRAME_14+(2*SZREG))(%r1);	/* restore r14 */	\
	ldreg	%r13,(FRAME_13+(2*SZREG))(%r1);	/* restore r13 */	\
	ldreg	%r12,(FRAME_12+(2*SZREG))(%r1);	/* restore r12 */	\
	ldreg	%r11,(FRAME_11+(2*SZREG))(%r1);	/* restore r11 */	\
	ldreg	%r10,(FRAME_10+(2*SZREG))(%r1);	/* restore r10 */	\
	ldreg	%r9,(FRAME_9+(2*SZREG))(%r1);	/* restore r9 */	\
	ldreg	%r8,(FRAME_8+(2*SZREG))(%r1);	/* restore r8 */	\
	ldreg	%r7,(FRAME_7+(2*SZREG))(%r1);	/* restore r7 */	\
	ldreg	%r6,(FRAME_6+(2*SZREG))(%r1);	/* restore r6 */	\
	ldreg	%r5,(FRAME_5+(2*SZREG))(%r1);	/* restore r5 */	\
	ldreg	%r4,(FRAME_4+(2*SZREG))(%r1);	/* restore r4 */	\
	ldreg	%r3,(FRAME_3+(2*SZREG))(%r1);	/* restore r3 */	\
	ldreg	%r2,(FRAME_2+(2*SZREG))(%r1);	/* restore r2 */	\
	ldreg	%r0,(FRAME_0+(2*SZREG))(%r1);	/* restore r0 */	\
	ldreg	%r1,(FRAME_1+(2*SZREG))(%r1);	/* restore old sp in r1 */ \
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
	bf	17,1f;			/* branch if PSL_PR is false */	\
/* Restore user SRs */							\
	RESTORE_USER_SRS(%r2,%r3);					\
1:	mfsprg1	%r2;			/* restore cr */		\
	mtcr	%r2;							\
	GET_CPUINFO(%r2);						\
	ldreg	%r3,(savearea+CPUSAVE_SRR0)(%r2);			\
	mtsrr0	%r3;							\
	ldreg	%r3,(savearea+CPUSAVE_SRR1)(%r2);			\
	lis	%r2,0x1000;						\
	rldicr  %r2,%r2,32,31;						\
	or	%r3,%r3,%r2;						\
	mtsrr1	%r3;							\
	mfsprg2	%r2;			/* restore r2 & r3 */		\
	mfsprg3	%r3
#else
#define	FRAME_LEAVE(savearea)						\
/* Now restore regs: */							\
	ldreg	%r2,(FRAME_SRR0+(2*SZREG))(%r1);			\
	ldreg	%r3,(FRAME_SRR1+(2*SZREG))(%r1);			\
	ldreg	%r4,(FRAME_CTR+(2*SZREG))(%r1);				\
	ldint	%r5,(FRAME_XER+(2*SZREG))(%r1);				\
	ldreg	%r6,(FRAME_LR+(2*SZREG))(%r1);				\
	RESTORE_MQ(%r1,%r8);						\
	RESTORE_VRSAVE(%r1,%r9);					\
	GET_CPUINFO(%r7);						\
	streg	%r2,(savearea+CPUSAVE_SRR0)(%r7); /* save SRR0 */	\
	streg	%r3,(savearea+CPUSAVE_SRR1)(%r7); /* save SRR1 */	\
	ldint	%r7,(FRAME_CR+(2*SZREG))(%r1);				\
	mtctr	%r4;							\
	mtxer	%r5;							\
	mtlr	%r6;							\
	mtsprg1	%r7;			/* save cr */			\
	ldreg	%r31,(FRAME_31+(2*SZREG))(%r1);	/* restore r31 */	\
	ldreg	%r30,(FRAME_30+(2*SZREG))(%r1);	/* restore r30 */	\
	ldreg	%r29,(FRAME_29+(2*SZREG))(%r1);	/* restore r29 */	\
	ldreg	%r28,(FRAME_28+(2*SZREG))(%r1);	/* restore r28 */	\
	ldreg	%r27,(FRAME_27+(2*SZREG))(%r1);	/* restore r27 */	\
	ldreg	%r26,(FRAME_26+(2*SZREG))(%r1);	/* restore r26 */	\
	ldreg	%r25,(FRAME_25+(2*SZREG))(%r1);	/* restore r25 */	\
	ldreg	%r24,(FRAME_24+(2*SZREG))(%r1);	/* restore r24 */	\
	ldreg	%r23,(FRAME_23+(2*SZREG))(%r1);	/* restore r23 */	\
	ldreg	%r22,(FRAME_22+(2*SZREG))(%r1);	/* restore r22 */	\
	ldreg	%r21,(FRAME_21+(2*SZREG))(%r1);	/* restore r21 */	\
	ldreg	%r20,(FRAME_20+(2*SZREG))(%r1);	/* restore r20 */	\
	ldreg	%r19,(FRAME_19+(2*SZREG))(%r1);	/* restore r19 */	\
	ldreg	%r18,(FRAME_18+(2*SZREG))(%r1);	/* restore r18 */	\
	ldreg	%r17,(FRAME_17+(2*SZREG))(%r1);	/* restore r17 */	\
	ldreg	%r16,(FRAME_16+(2*SZREG))(%r1);	/* restore r16 */	\
	ldreg	%r15,(FRAME_15+(2*SZREG))(%r1);	/* restore r15 */	\
	ldreg	%r14,(FRAME_14+(2*SZREG))(%r1);	/* restore r14 */	\
	ldreg	%r13,(FRAME_13+(2*SZREG))(%r1);	/* restore r13 */	\
	ldreg	%r12,(FRAME_12+(2*SZREG))(%r1);	/* restore r12 */	\
	ldreg	%r11,(FRAME_11+(2*SZREG))(%r1);	/* restore r11 */	\
	ldreg	%r10,(FRAME_10+(2*SZREG))(%r1);	/* restore r10 */	\
	ldreg	%r9,(FRAME_9+(2*SZREG))(%r1);	/* restore r9 */	\
	ldreg	%r8,(FRAME_8+(2*SZREG))(%r1);	/* restore r8 */	\
	ldreg	%r7,(FRAME_7+(2*SZREG))(%r1);	/* restore r7 */	\
	ldreg	%r6,(FRAME_6+(2*SZREG))(%r1);	/* restore r6 */	\
	ldreg	%r5,(FRAME_5+(2*SZREG))(%r1);	/* restore r5 */	\
	ldreg	%r4,(FRAME_4+(2*SZREG))(%r1);	/* restore r4 */	\
	ldreg	%r3,(FRAME_3+(2*SZREG))(%r1);	/* restore r3 */	\
	ldreg	%r2,(FRAME_2+(2*SZREG))(%r1);	/* restore r2 */	\
	ldreg	%r0,(FRAME_0+(2*SZREG))(%r1);	/* restore r0 */	\
	ldreg	%r1,(FRAME_1+(2*SZREG))(%r1);	/* restore old sp in r1 */ \
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
	bf	17,1f;			/* branch if PSL_PR is false */	\
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
#endif /* PPC_OEA64_BRIDGE */

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
	bt	17,trapstart		/* branch is user mode */
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
	bf	17,k_trap		/* branch if PSL_PR is false */
	GET_CPUINFO(%r1)		/* get cpu_info for this cpu */
u_trap:
	ldptr	%r1,CI_CURPCB(%r1)
	addi	%r1,%r1,USPACE		/* stack is top of user struct */

/*
 * Now the common trap catching code.
 */

	RESTORE_KERN_SRS(%r30,%r31)	/* First enable KERNEL mapping */

k_trap:
	FRAME_SETUP(CI_TEMPSAVE)
trapagain:
/* Now we can recover interrupts again: */
	mfmsr	%r7
	ldreg	%r6, (FRAME_SRR1+(2*SZREG))(%r1)
	andi.	%r6,%r6,(PSL_EE|PSL_ME|PSL_RI)@l
	or	%r7,%r7,%r6
	mtmsr	%r7
	isync
/* Call C trap code: */
	addi	%r3,%r1,(2*SZREG)
	bl	_C_LABEL(trap)
/* LINTSTUB: Var: int trapexit[1]; */
	.globl	trapexit
trapexit:
/* Disable interrupts: */
	mfmsr	%r3
	andi.	%r3,%r3,~PSL_EE@l
	mtmsr	%r3
/* Test AST pending: */
	ldreg	%r5,(FRAME_SRR1+(2*SZREG))(%r1)
	mtcr	%r5
	bf	17,1f			/* branch if PSL_PR is false */
	GET_CPUINFO(%r3)
	ldint	%r4,CI_ASTPENDING(%r3)
	andi.	%r4,%r4,1
	beq	1f
	li	%r6,EXC_AST
	stint	%r6,(FRAME_EXC+(2*SZREG))(%r1)
	b	trapagain
1:
	FRAME_LEAVE(CI_TEMPSAVE)
	RFI

/* 
 * Trap handler for syscalls (EXC_SC)
 */
/* LINTSTUB: Var: int sctrap[1], scsize[1]; */
	.globl	_C_LABEL(sctrap),_C_LABEL(scsize),_C_LABEL(sctrapexit)
_C_LABEL(sctrap):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1) /* save r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1) /* save r20 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1) /* save r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1) /* save r31 */
#if defined (PPC_OEA64_BRIDGE)
	mfmsr	%r28
	clrldi  %r28,%r28,1
	mtmsrd	%r28
	isync
#endif
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	bla	s_sctrap
	_C_LABEL(scsize) = .-_C_LABEL(sctrap)

s_sctrap:
	GET_CPUINFO(%r1)
	ldptr	%r1,CI_CURPCB(%r1)
	addi	%r1,%r1,USPACE		/* stack is top of user struct */
	RESTORE_KERN_SRS(%r30,%r31)	/* First enable KERNEL mapping */
	FRAME_SETUP(CI_TEMPSAVE)
/* Now we can recover interrupts again: */
	mfmsr	%r7
	ori	%r7,%r7,(PSL_EE|PSL_ME|PSL_RI)@l
	mtmsr	%r7
	isync
	addi	%r3,%r1,(2*SZREG)
/* Call the appropriate syscall handler: */
	GET_CPUINFO(%r4)
	ldptr	%r4,CI_CURLWP(%r4)
	ldptr	%r4,L_PROC(%r4)
	ldptr	%r4,P_MD_SYSCALL(%r4)
	mtctr	%r4
	bctrl
_C_LABEL(sctrapexit):
/* Disable interrupts: */
	mfmsr	%r3
	andi.	%r3,%r3,~PSL_EE@l
	mtmsr	%r3
/* Test AST pending: */
	GET_CPUINFO(%r3)
	ldint	%r4,CI_ASTPENDING(%r3)
	andi.	%r4,%r4,1
	beq	1f
	li	%r6,EXC_AST
	stint	%r6,(FRAME_EXC+(2*SZREG))(%r1)
	b	trapagain
1:
	FRAME_LEAVE(CI_TEMPSAVE)
	RFI

/*
 * External interrupt second level handler
 */
#define	INTRENTER							\
/* Save non-volatile registers: */					\
	stptru	%r1,-IFRAMELEN(%r1);	/* temporarily */		\
	streg	%r0,IFRAME_R0(%r1);					\
	mfsprg1	%r0;			/* get original SP */		\
	streg	%r0,IFRAME_R1(%r1);	/* and store it */		\
	streg	%r3,IFRAME_R3(%r1);					\
	streg	%r4,IFRAME_R4(%r1);					\
	streg	%r5,IFRAME_R5(%r1);					\
	streg	%r6,IFRAME_R6(%r1);					\
	streg	%r7,IFRAME_R7(%r1);					\
	streg	%r8,IFRAME_R8(%r1);					\
	streg	%r9,IFRAME_R9(%r1);					\
	streg	%r10,IFRAME_R10(%r1);					\
	streg	%r11,IFRAME_R11(%r1);					\
	streg	%r12,IFRAME_R12(%r1);					\
	streg	%r28,IFRAME_LR(%r1);	/* saved LR */			\
	stint	%r29,IFRAME_CR(%r1);	/* saved CR */			\
	stint	%r30,IFRAME_XER(%r1);	/* saved XER */			\
	GET_CPUINFO(%r4);						\
	ldreg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r4); /* restore r28 */	\
	ldreg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r4); /* restore r29 */	\
	ldreg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r4); /* restore r30 */	\
	ldreg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r4); /* restore r31 */	\
	mfctr	%r6;							\
	ldint	%r5,CI_INTRDEPTH(%r4);					\
	mfsrr0	%r4;							\
	mfsrr1	%r3;							\
	streg	%r6,IFRAME_CTR(%r1);					\
	stint	%r5,IFRAME_INTR_DEPTH(%r1);				\
	streg	%r4,IFRAME_SRR0(%r1);					\
	streg	%r3,IFRAME_SRR1(%r1);					\
	mtcr	%r3;							\
	bf	17,99f;	/* branch if PSL_PR is false */		\
/* interrupts are recoverable here, and enable translation */		\
	RESTORE_KERN_SRS(%r3,%r4);					\
99:	mfmsr	%r5;							\
	ori	%r5,%r5,(PSL_IR|PSL_DR|PSL_RI);				\
	mtmsr	%r5;							\
	isync

/* LINTSTUB: Var: int extint_call[1]; */
	.globl	_C_LABEL(extint_call)
extintr:
	INTRENTER
	mr	%r3,%r1			/* make intrframe available */
_C_LABEL(extint_call):
	bl	_C_LABEL(extint_call)	/* to be filled in later */

intr_exit:
/* Disable interrupts (should already be disabled) and MMU here: */
	mfmsr	%r3
	andi.	%r3,%r3,~(PSL_EE|PSL_ME|PSL_RI|PSL_DR|PSL_IR)@l
	mtmsr	%r3
	isync

/* restore possibly overwritten registers: */
	ldreg	%r12,IFRAME_R12(%r1)
	ldreg	%r11,IFRAME_R11(%r1)
	ldreg	%r10,IFRAME_R10(%r1)
	ldreg	%r9,IFRAME_R9(%r1)
	ldreg	%r8,IFRAME_R8(%r1)
	ldreg	%r7,IFRAME_R7(%r1)
	ldreg	%r6,IFRAME_SRR1(%r1)
#if defined (PPC_OEA64_BRIDGE)
	lis	%r5,0x1000
	rldicr  %r5,%r5,32,31
	or	%r6,%r6,%r5
#endif
	ldreg	%r5,IFRAME_SRR0(%r1)
	ldreg	%r4,IFRAME_CTR(%r1)
	ldint	%r3,IFRAME_XER(%r1)
	mtsrr1	%r6
	mtsrr0	%r5
	mtctr	%r4
	mtxer	%r3

	GET_CPUINFO(%r5)
	ldint	%r4,CI_INTRDEPTH(%r5)
	addi	%r4,%r4,-1		/* adjust reentrancy count */
	stint	%r4,CI_INTRDEPTH(%r5)

/* Returning to user mode? */
	mtcr	%r6			/* saved SRR1 */
	bf	17,1f			/* branch if PSL_PR is false */

	RESTORE_USER_SRS(%r3,%r4)
	ldint	%r3,CI_ASTPENDING(%r5)	/* Test AST pending */
	andi.	%r3,%r3,1
	beq	1f

/* Setup for entry to realtrap: */
	ldreg	%r3,IFRAME_R1(%r1)	/* get saved SP */
	mtsprg1	%r3
	li	%r6,EXC_AST
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r5) /* establish tempsave r28 */
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r5) /* establish tempsave r29 */
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r5) /* establish tempsave r30 */
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r5) /* establish tempsave r31 */
	mtlr	%r6
	ldreg	%r28,IFRAME_LR(%r1)	/* saved LR */
	ldint	%r29,IFRAME_CR(%r1)	/* saved CR */
	ldreg	%r6,IFRAME_R6(%r1)
	ldreg	%r5,IFRAME_R5(%r1)
	ldreg	%r4,IFRAME_R4(%r1)
	ldreg	%r3,IFRAME_R3(%r1)
	ldreg	%r0,IFRAME_R0(%r1)
	b	realtrap
1:
/* Here is the normal exit of extintr: */
	ldint	%r5,IFRAME_CR(%r1)
	ldreg	%r6,IFRAME_LR(%r1)
	mtcr	%r5
	mtlr	%r6
	ldreg	%r6,IFRAME_R6(%r1)
	ldreg	%r5,IFRAME_R5(%r1)
	ldreg	%r4,IFRAME_R4(%r1)
	ldreg	%r3,IFRAME_R3(%r1)
	ldreg	%r0,IFRAME_R0(%r1)
	ldreg	%r1,IFRAME_R1(%r1)
	RFI

/*
 * Decrementer interrupt second level handler
 */
decrintr:
	INTRENTER
	addi	%r3,%r1,(2*SZREG)	/* intr frame -> clock frame */
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
	addi	%r3,%r1,(2*SZREG)
	bl	_C_LABEL(ddb_trap_glue)
	or.	%r3,%r3,%r3
	bne	ddbleave
/* This wasn't for DDB, so switch to real trap: */
	ldint	%r3,(FRAME_EXC+(2*SZREG))(%r1)	/* save exception */
	GET_CPUINFO(%r4)
	streg	%r3,(CI_DDBSAVE+CPUSAVE_R31)(%r4)
	FRAME_LEAVE(CI_DDBSAVE)
	mtsprg1	%r1			/* prepare for entrance to realtrap */
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)
	mflr	%r28
	mfcr	%r29
	ldreg	%r31,(CI_DDBSAVE+CPUSAVE_R31)(%r1)
	mtlr	%r31
	mfsprg1	%r1
	b	realtrap
ddbleave:
	FRAME_LEAVE(CI_DDBSAVE)
	RFI
#endif /* DDB || KGDB */

#ifdef IPKDB
/*
 * Deliberate entry to ipkdbtrap
 */
	.globl	_C_LABEL(ipkdb_trap)
_C_LABEL(ipkdb_trap):
	mtsprg1	%r1
	mfmsr	%r3
	mtsrr1	%r3
	andi.	%r3,%r3,~(PSL_EE|PSL_ME)@l
	mtmsr	%r3			/* disable interrupts */
	isync
	GET_CPUINFO(%r3)
	streg	%r28,(CI_IPKDBSAVE+CPUSAVE_R28)(%r3)
	streg	%r29,(CI_IPKDBSAVE+CPUSAVE_R29)(%r3)
	streg	%r30,(CI_IPKDBSAVE+CPUSAVE_R30)(%r3)
	streg	%r31,(CI_IPKDBSAVE+CPUSAVE_R31)(%r3)
	mflr	%r28
	li	%r29,EXC_BPT
	mtlr	%r29
	mfcr	%r29
	mtsrr0	%r28

/*
 * Now the ipkdb trap catching code.
 */
ipkdbtrap:
	FRAME_SETUP(ipkdbsave)
/* Call C trap code: */
	addi	%r3,%r1,(2*SZREG)
	bl	_C_LABEL(ipkdb_trap_glue)
	or.	%r3,%r3,%r3
	bne	ipkdbleave
/* This wasn't for IPKDB, so switch to real trap: */
	ldint	%r3,(FRAME_EXC+(2*SZREG))(%r1)	/* save exception */
	GET_CPUINFO(%r4)
	streg	%r3,(CI_IPKDBSAVE+CPUSAVE_R31)(%r4)
	FRAME_LEAVE(CI_IPKDBSAVE)
	mtsprg1	%r1			/* prepare for entrance to realtrap */
	GET_CPUINFO(%r1)
	streg	%r28,(CI_TEMPSAVE+CPUSAVE_R28)(%r1)
	streg	%r29,(CI_TEMPSAVE+CPUSAVE_R29)(%r1)
	streg	%r30,(CI_TEMPSAVE+CPUSAVE_R30)(%r1)
	streg	%r31,(CI_TEMPSAVE+CPUSAVE_R31)(%r1)
	mflr	%r28
	mfcr	%r29
	ldreg	%r31,(CI_IPKDBSAVE+CPUSAVE_R31)(%r1)
	mtlr	%r31
	mfsprg1	%r1
	b	realtrap
ipkdbleave:
	FRAME_LEAVE(CI_IPKDBSAVE)
	RFI

ipkdbfault:
	ba	_ipkdbfault
_ipkdbfault:
	mfsrr0	%r3
	addi	%r3,%r3,4
	mtsrr0	%r3
	li	%r3,-1
	RFI

/*
 * int ipkdbfbyte(unsigned char *p)
 */
	.globl	_C_LABEL(ipkdbfbyte)
_C_LABEL(ipkdbfbyte):
	li	%r9,EXC_DSI		/* establish new fault routine */
	ldint	%r5,0(%r9)
	lis	%r6,ipkdbfault@ha
	ldint	%r6,ipkdbfault@l(%r6)
	stint	%r6,0(%r9)
#ifdef	IPKDBUSERHACK
	lis	%r8,_C_LABEL(ipkdbsr)@ha
	ldreg	%r8,_C_LABEL(ipkdbsr)@l(%r8)
	mtsr	USER_SR,8
	isync
#endif
	dcbst	%r0,%r9			/* flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	lbz	%r3,0(%r3)		/* fetch data */
	stint	%r5,0(%r9)		/* restore previous fault handler */
	dcbst	%r0,%r9			/* and flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	blr

/*
 * int ipkdbsbyte(unsigned char *p, int c)
 */
	.globl	_C_LABEL(ipkdbsbyte)
_C_LABEL(ipkdbsbyte):
	li	%r9,EXC_DSI		/* establish new fault routine */
	ldint	%r5,0(%r9)
	lis	%r6,ipkdbfault@ha
	ldint	%r6,ipkdbfault@l(%r6)
	stint	%r6,0(%r9)
#ifdef	IPKDBUSERHACK
	lis	%r8,_C_LABEL(ipkdbsr)@ha
	ldreg	%r8,_C_LABEL(ipkdbsr)@l(%r8)
	mtsr	USER_SR,%r8
	isync
#endif
	dcbst	%r0,%r9			/* flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	mr	%r6,%r3
	li	%r3,0
	stb	%r4,0(%r6)
	dcbst	%r0,%r6			/* Now do appropriate flushes
					   to data... */
	sync
	icbi	%r0,%r6			/* and instruction caches */
	stint	%r5,0(%r9)		/* restore previous fault handler */
	dcbst	%r0,%r9			/* and flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	blr
#endif	/* IPKDB */
	.globl	_C_LABEL(trapend)
_C_LABEL(trapend):
