/*	$NetBSD: trap_subr.S,v 1.42 2003/07/31 07:51:16 matt Exp $	*/

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
	stw	b,(FRAME_VRSAVE+(2*SZREG))(tf);

#define RESTORE_VRSAVE(tf,b)						\
	lwz	b,(FRAME_VRSAVE+(2*SZREG))(tf);				\
	mtspr	SPR_VRSAVE,b;
#else
#define SAVE_VRSAVE(tf,b)
#define RESTORE_VRSAVE(tf,b)
#endif
#define	RESTORE_SRS(pmap,sr)	mtsr	0,sr; \
	lwz	sr,4(pmap);	mtsr	1,sr; \
	lwz	sr,8(pmap);	mtsr	2,sr; \
	lwz	sr,12(pmap);	mtsr	3,sr; \
	lwz	sr,16(pmap);	mtsr	4,sr; \
	lwz	sr,20(pmap);	mtsr	5,sr; \
	lwz	sr,24(pmap);	mtsr	6,sr; \
	lwz	sr,28(pmap);	mtsr	7,sr; \
	lwz	sr,32(pmap);	mtsr	8,sr; \
	lwz	sr,36(pmap);	mtsr	9,sr; \
	lwz	sr,40(pmap);	mtsr	10,sr; \
	lwz	sr,44(pmap);	mtsr	11,sr; \
	lwz	sr,48(pmap);	mtsr	12,sr; \
	lwz	sr,52(pmap);	mtsr	13,sr; \
	lwz	sr,56(pmap);	mtsr	14,sr; \
	lwz	sr,60(pmap);	mtsr	15,sr; isync;

/*
 * User SRs are loaded through a pointer to the current pmap.
 */
#define RESTORE_USER_SRS(pmap,sr) \
	GET_CPUINFO(pmap); \
	lwz	pmap,CI_CURPM(pmap); \
	lwzu	sr,PM_SR(pmap); \
	RESTORE_SRS(pmap,sr)

/*
 * Kernel SRs are loaded directly from kernel_pmap_
 */
#define RESTORE_KERN_SRS(pmap,sr) \
	lis	pmap,_C_LABEL(kernel_pmap_)@ha;	\
	lwzu	sr,_C_LABEL(kernel_pmap_)+PM_SR@l(pmap); \
	RESTORE_SRS(pmap,sr)

/*
 * Save/restore MPC601 MQ register.
 * Note: oea_init() relies on this instruction sequence.
 */
#define	SAVE_MQ(tf,b)							\
	mfspr	b,SPR_MQ;						\
	stw	b,(FRAME_MQ+(2*SZREG))(tf);

#define	RESTORE_MQ(tf,b)						\
	lwz	b,(FRAME_MQ+(2*SZREG))(tf);				\
	mtspr	SPR_MQ,b;

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
	stmw	%r28,CI_TEMPSAVE(%r1)	/* free r28-r31 */
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
	stmw	%r28,CI_TEMPSAVE(%r1)	/* free r28-r31 */
	mfdar	%r30
	mfdsisr	%r31
	stmw	%r30,(CI_TEMPSAVE+(4*SZREG))(%r1)
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
	stmw	%r28,CI_DISISAVE(%r1)	/* free r28-r31 */
	mfsprg1	%r1
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	mtsprg2	%r30			/* in SPRG2 */
	mfsrr1	%r31			/* test kernel mode */
	mtcr	%r31
	bc	12,17,1f		/* branch if PSL_PR is set */
	mfdar	%r31			/* get fault address */
	rlwinm	%r31,%r31,7,25,28	/* get segment * 8 */

	/* get batu */
	addis	%r31,%r31,_C_LABEL(battable)@ha
	lwz	%r30,_C_LABEL(battable)@l(%r31)
	mtcr	%r30
	bc	4,30,1f			/* branch if supervisor valid is
					   false */
	/* get batl */
	lwz	%r31,_C_LABEL(battable)+SZREG@l(%r31)
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
	mfsprg2	%r30			/* restore XER */
	mtxer	%r30
	mtcr	%r29			/* restore CR */
	mtsprg1	%r1
	GET_CPUINFO(%r1)
	lmw	%r28,CI_DISISAVE(%r1)	/* restore r28-r31 */
	mfsprg1	%r1
	rfi				/* return to trapped code */
1:
	mflr	%r28			/* save LR */
	mtsprg1	%r1			/* save SP */
	bla	disitrap
_C_LABEL(dsisize) = .-_C_LABEL(dsitrap)

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
	stmw	%r28,CI_DISISAVE(%r1)	/* free r28-r31 */
	mfsprg1	%r1
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	mtsprg2	%r30			/* in SPRG2 */
	mfsrr1	%r31			/* test kernel mode */
	mtcr	%r31
	bc	12,17,1f		/* branch if PSL_PR is set */
	mfdar	%r31			/* get fault address */
	rlwinm	%r31,%r31,12,20,28	/* get "segment" battable offset */

	/* get batl */
	addis	%r31,%r31,_C_LABEL(battable)@ha
	lwz	%r30,_C_LABEL(battable)+SZREG@l(%r31)
	mtcr	%r30
	bc	4,25,1f			/* branch if Valid is is false,
					   presently assumes supervisor only */

	/* get batu */
	lwz	%r31,_C_LABEL(battable)@l(%r31)
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
	lmw	%r28,CI_DISISAVE(%r1)	/* restore r28-r31 */
	mfsprg1	%r1
	rfi				/* return to trapped code */
1:
	mflr	%r28			/* save LR */
	mtsprg1	%r1
	bla	disitrap
_C_LABEL(dsi601size) = .-_C_LABEL(dsi601trap)

/*
 * This one for the external interrupt handler.
 */
/* LINTSTUB: Var: int extint[1], extsize[1]; */
	.globl	_C_LABEL(extint),_C_LABEL(extsize)
_C_LABEL(extint):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	stmw	%r28,CI_TEMPSAVE(%r1)	/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	lwz	%r31,CI_INTRDEPTH(%r1)	/* were we already running on intstk? */
	addic.	%r31,%r31,1
	stw	%r31,CI_INTRDEPTH(%r1)
	lwz	%r1,CI_INTSTK(%r1)		/* get interrupt stack */
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
	stmw	%r28,CI_TEMPSAVE(%r1)	/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	lwz	%r31,CI_INTRDEPTH(%r1)	/* were we already running on intstk? */
	addic.	%r31,%r31,1
	stw	%r31,CI_INTRDEPTH(%r1)
	lwz	%r1,CI_INTSTK(%r1)		/* get interrupt stack */
	beq	1f
	mfsprg1	%r1			/* yes, get old SP */
1:
	ba	decrintr
_C_LABEL(decrsize) = .-_C_LABEL(decrint)

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
	lwzu	%r1,8(%r2)		/* get next pte */
	cmpl	%cr0,%r1,%r3		/* see if found pte */
	bdneq	2b			/* loop if not eq */
	bne	3f			/* not found */
	lwz	%r1,4(%r2)		/* load tlb entry lower word */
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
	rfi

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
	lwzu	%r1,8(%r2)		/* get next pte */
	cmpl	%cr0,%r1,%r3		/* see if found pte */
	bdneq	2b			/* loop if not eq */
	bne	3f			/* not found */
	lwz	%r1,4(%r2)		/* load tlb entry lower word */
	mtctr	%r0			/* restore counter */
	mfspr	%r0,SPR_DMISS		/* get the miss address for the tlbld */
	mfsrr1	%r3			/* get the saved cr0 bits */
	mtcrf	0x80,%r3		/* and restore */
	ori	%r1,%r1,PTE_REF		/* set the reference bit */
	mtspr	SPR_RPA,%r1		/* set the pte */
	srwi	%r1,%r1,8		/* get byte 7 of pte */
	tlbld	%r0			/* load the dtlb */
	stb	%r1,6(%r2)		/* update page table */
	rfi

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
	lwzu	%r1,8(%r2)		/* get next pte */
	cmpl	%cr0,%r1,%r3		/* see if found pte */
	bdneq	2b			/* loop if not eq */
	bne	3f			/* not found */
	lwz	%r1,4(%r2)		/* load tlb entry lower word */
	andi.	%r3,%r1,PTE_CHG		/* check the C-bit */
	beq	4f
5:
	mtctr	%r0			/* restore counter */
	mfspr	%r0,SPR_DMISS		/* get the miss address for the tlbld */
	mfsrr1	%r3			/* get the saved cr0 bits */
	mtcrf	0x80,%r3		/* and restore */
	mtspr	SPR_RPA,%r1		/* set the pte */
	tlbld	%r0			/* load the dtlb */
	rfi

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
	lwz	%r1,4(%r2)		/* reload tlb entry */
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

#if defined(DDB) || defined(KGDB)
#define	ddbsave	0xde0		/* primary save area for DDB */
/*
 * In case of DDB we want a separate trap catcher for it
 */
	.local	ddbstk
	.comm	ddbstk,INTSTK,8		/* ddb stack */

/* LINTSTUB: Var: int ddblow[1], ddbsize[1]; */
	.globl	_C_LABEL(ddblow),_C_LABEL(ddbsize)
_C_LABEL(ddblow):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	stmw	%r28,CI_DDBSAVE(%r1)	/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
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
	stmw	%r28,CI_IPKDBSAVE(%r1)	/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	lis	%r1,ipkdbstk+INTSTK@ha	/* get new SP */
	addi	%r1,%r1,ipkdbstk+INTSTK@l
	bla	ipkdbtrap
_C_LABEL(ipkdbsize) = .-_C_LABEL(ipkdblow)
#endif	/* IPKDB */

#ifdef	CPU601_KERN_ENTRY_HOOK
#define	CPU601_KERN_ENTRY(_s1_,_s2_)					\
	mfpvr	_s1_;							\
	srwi	_s1_,_s1_,16;						\
	cmpi	%cr0,_s1_,MPC601;					\
	bne	98f;			/* skip if not 601 */		\
	CPU601_KERN_ENTRY_HOOK(_s1_,_s2_);				\
98:
#else
#define	CPU601_KERN_ENTRY(_s1_,_s2_)	/* nothing */
#endif

#ifdef CPU601_KERN_LEAVE_HOOK
#define	CPU601_KERN_LEAVE(_pmap_,_s1_)					\
	mfpvr	_s1_;							\
	srwi	_s1_,_s1_,16;						\
	cmpi	%cr0,_s1_,MPC601;					\
	bne	99f;			/* skip if not 601 */		\
	CPU601_KERN_LEAVE_HOOK(_pmap_,_s1_);				\
	li	_s1_,0;							\
	mtibatl	0,_s1_;			/* obliterate BATs */		\
	mtibatl	1,_s1_;							\
	mtibatl	2,_s1_;							\
	mtibatl	3,_s1_;							\
99:
#else
#define	CPU601_KERN_LEAVE(_pmap_,_s1_)	/* nothing */
#endif

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
	stw	%r30,(savearea+(6*SZREG))(%r31);	/* save SRR0 */	\
	mfsrr1	%r30;							\
	stw	%r30,(savearea+(7*SZREG))(%r31);	/* save SRR1 */	\
	mfmsr	%r30;							\
	ori	%r30,%r30,(PSL_DR|PSL_IR);	/* turn on relocation */ \
	mtmsr	%r30;			/* stack can be accesed now */	\
	isync;								\
	mfsprg1	%r31;			/* get saved SP */		\
	stwu	%r31,-FRAMELEN(%r1);	/* save it in the callframe */	\
	stw	%r0,(FRAME_0+(2*SZREG))(%r1);	/* save R0 in the trapframe */ \
	stw	%r31,(FRAME_1+(2*SZREG))(%r1);	/* save SP in the trapframe */ \
	stw	%r2,(FRAME_2+(2*SZREG))(%r1);	/* save R2 in the trapframe */ \
	stw	%r28,(FRAME_LR+(2*SZREG))(%r1);				\
	stw	%r29,(FRAME_CR+(2*SZREG))(%r1);				\
	GET_CPUINFO(%r2);						\
	lmw	%r28,savearea(%r2);		/* get saved r28-r31 */	\
	stmw	%r3,(FRAME_3+(2*SZREG))(%r1);	/* save r3-r31 */	\
	lmw	%r28,(savearea+(4*SZREG))(%r2);	/* get DAR/DSISR/SRR0/SRR1 */ \
	mfxer	%r3;							\
	mfctr	%r4;							\
	mflr	%r5;							\
	andi.	%r5,%r5,0xff00;						\
	stw	%r3,(FRAME_XER+(2*SZREG))(%r1);				\
	stw	%r4,(FRAME_CTR+(2*SZREG))(%r1);				\
	stw	%r5,(FRAME_EXC+(2*SZREG))(%r1);				\
	SAVE_VRSAVE(%r1,%r6);						\
	SAVE_MQ(%r1,%r7);						\
	stw	%r28,(FRAME_DAR+(2*SZREG))(%r1);			\
	stw	%r29,(FRAME_DSISR+(2*SZREG))(%r1);			\
	stw	%r30,(FRAME_SRR0+(2*SZREG))(%r1);			\
	stw	%r31,(FRAME_SRR1+(2*SZREG))(%r1)

#define	FRAME_LEAVE(savearea)						\
/* Now restore regs: */							\
	lwz	%r2,(FRAME_SRR0+(2*SZREG))(%r1);			\
	lwz	%r3,(FRAME_SRR1+(2*SZREG))(%r1);			\
	lwz	%r4,(FRAME_CTR+(2*SZREG))(%r1);				\
	lwz	%r5,(FRAME_XER+(2*SZREG))(%r1);				\
	lwz	%r6,(FRAME_LR+(2*SZREG))(%r1);				\
	RESTORE_MQ(%r1,%r8);						\
	RESTORE_VRSAVE(%r1,%r9);					\
	GET_CPUINFO(%r7);						\
	stw	%r2,savearea(%r7);					\
	stw	%r3,(savearea+SZREG)(%r7);				\
	lwz	%r7,(FRAME_CR+(2*SZREG))(%r1);				\
	mtctr	%r4;							\
	mtxer	%r5;							\
	mtlr	%r6;							\
	mtsprg1	%r7;			/* save cr */			\
	lmw	%r2,(FRAME_2+(2*SZREG))(%r1);				\
	lwz	%r0,(FRAME_0+(2*SZREG))(%r1);	/* restore r0 */	\
	lwz	%r1,(FRAME_1+(2*SZREG))(%r1);	/* restore old sp in r1 */ \
	mtsprg2	%r2;			/* save r2 & r3 */		\
	mtsprg3	%r3;							\
/* Disable translation, machine check and recoverability: */		\
	mfmsr	%r2;							\
	andi.	%r2,%r2,~(PSL_DR|PSL_IR|PSL_ME|PSL_RI)@l;		\
	mtmsr	%r2;							\
	isync;								\
/* Decide whether we return to user mode: */				\
	GET_CPUINFO(%r2);						\
	lwz	%r3,(savearea+SZREG)(%r2);				\
	mtcr	%r3;							\
	bc	4,17,1f;		/* branch if PSL_PR is false */	\
/* Restore user SRs */							\
	CPU601_KERN_LEAVE(%r2,%r3);					\
	RESTORE_USER_SRS(%r2,%r3);					\
1:	mfsprg1	%r2;			/* restore cr */		\
	mtcr	%r2;							\
	GET_CPUINFO(%r2);						\
	lwz	%r3,savearea(%r2);					\
	mtsrr0	%r3;							\
	lwz	%r3,(savearea+SZREG)(%r2);				\
	mtsrr1	%r3;							\
	mfsprg2	%r2;			/* restore r2 & r3 */		\
	mfsprg3	%r3

/*
 * Preamble code for DSI/ISI traps
 */
disitrap:
	GET_CPUINFO(%r1)
	lmw	%r30,CI_DISISAVE(%r1)
	stmw	%r30,CI_TEMPSAVE(%r1)
	lmw	%r30,(CI_DISISAVE+(2*SZREG))(%r1)
	stmw	%r30,(CI_TEMPSAVE+(2*SZREG))(%r1)
	mfdar	%r30
	mfdsisr	%r31
	stmw	%r30,(CI_TEMPSAVE+(4*SZREG))(%r1)
	.globl	_C_LABEL(trapstart)
_C_LABEL(trapstart):
realtrap:
/* Test whether we already had PR set */
	mfsrr1	%r1
	mtcr	%r1
	mfsprg1	%r1			/* restore SP (might have been
					   overwritten) */
s_trap:
	bc	4,17,k_trap		/* branch if PSL_PR is false */
	GET_CPUINFO(%r1)
	lwz	%r1,CI_CURPCB(%r1)
	addi	%r1,%r1,USPACE		/* stack is top of user struct */

/*
 * Now the common trap catching code.
 */

	RESTORE_KERN_SRS(%r30,%r31)	/* First enable KERNEL mapping */
	CPU601_KERN_ENTRY(%r30,%r31)

k_trap:
	FRAME_SETUP(CI_TEMPSAVE)
trapagain:
/* Now we can recover interrupts again: */
	mfmsr	%r7
	ori	%r7,%r7,(PSL_EE|PSL_ME|PSL_RI)@l
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
	lwz	%r5,(FRAME_SRR1+(2*SZREG))(%r1)
	mtcr	%r5
	bc	4,17,1f			/* branch if PSL_PR is false */
	GET_CPUINFO(%r3)
	lwz	%r4,CI_ASTPENDING(%r3)
	andi.	%r4,%r4,1
	beq	1f
	li	%r6,EXC_AST
	stw	%r6,(FRAME_EXC+(2*SZREG))(%r1)
	b	trapagain
1:
	FRAME_LEAVE(CI_TEMPSAVE)
	rfi

/* 
 * Trap handler for syscalls (EXC_SC)
 */
/* LINTSTUB: Var: int sctrap[1], scsize[1]; */
	.globl	_C_LABEL(sctrap),_C_LABEL(scsize),_C_LABEL(sctrapexit)
_C_LABEL(sctrap):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	stmw	%r28,CI_TEMPSAVE(%r1)	/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	bla	s_sctrap
	_C_LABEL(scsize) = .-_C_LABEL(sctrap)

s_sctrap:
	GET_CPUINFO(%r1)
	lwz	%r1,CI_CURPCB(%r1)
	addi	%r1,%r1,USPACE		/* stack is top of user struct */
	RESTORE_KERN_SRS(%r30,%r31)	/* First enable KERNEL mapping */
	CPU601_KERN_ENTRY(%r30,%r31)
	FRAME_SETUP(CI_TEMPSAVE)
/* Now we can recover interrupts again: */
	mfmsr	%r7
	ori	%r7,%r7,(PSL_EE|PSL_ME|PSL_RI)@l
	mtmsr	%r7
	isync
	addi	%r3,%r1,(2*SZREG)
/* Call the appropriate syscall handler: */
	GET_CPUINFO(%r4)
	lwz	%r4,CI_CURLWP(%r4)
	lwz	%r4,L_PROC(%r4)
	lwz	%r4,P_MD_SYSCALL(%r4)
	mtctr	%r4
	bctrl
_C_LABEL(sctrapexit):
/* Disable interrupts: */
	mfmsr	%r3
	andi.	%r3,%r3,~PSL_EE@l
	mtmsr	%r3
/* Test AST pending: */
	GET_CPUINFO(%r3)
	lwz	%r4,CI_ASTPENDING(%r3)
	andi.	%r4,%r4,1
	beq	1f
	li	%r6,EXC_AST
	stw	%r6,(FRAME_EXC+(2*SZREG))(%r1)
	b	trapagain
1:
	FRAME_LEAVE(CI_TEMPSAVE)
	rfi

/*
 * External interrupt second level handler
 */
#define	INTRENTER							\
/* Save non-volatile registers: */					\
	stwu	%r1,-IFRAMELEN(%r1);	/* temporarily */		\
	stw	%r0,IFRAME_R0(%r1);					\
	mfsprg1	%r0;			/* get original SP */		\
	stw	%r0,IFRAME_R1(%r1);	/* and store it */		\
	stw	%r3,IFRAME_R3(%r1);					\
	stw	%r4,IFRAME_R4(%r1);					\
	stw	%r5,IFRAME_R5(%r1);					\
	stw	%r6,IFRAME_R6(%r1);					\
	stw	%r7,IFRAME_R7(%r1);					\
	stw	%r8,IFRAME_R8(%r1);					\
	stw	%r9,IFRAME_R9(%r1);					\
	stw	%r10,IFRAME_R10(%r1);					\
	stw	%r11,IFRAME_R11(%r1);					\
	stw	%r12,IFRAME_R12(%r1);					\
	stw	%r28,IFRAME_LR(%r1);	/* saved LR */			\
	stw	%r29,IFRAME_CR(%r1);	/* saved CR */			\
	stw	%r30,IFRAME_XER(%r1);	/* saved XER */			\
	GET_CPUINFO(%r4);						\
	lmw	%r28,CI_TEMPSAVE(%r4);	/* restore r28-r31 */		\
	mfctr	%r6;							\
	lwz	%r5,CI_INTRDEPTH(%r4);					\
	mfsrr0	%r4;							\
	mfsrr1	%r3;							\
	stw	%r6,IFRAME_CTR(%r1);					\
	stw	%r5,IFRAME_INTR_DEPTH(%r1);				\
	stw	%r4,IFRAME_SRR0(%r1);					\
	stw	%r3,IFRAME_SRR1(%r1);					\
	mtcr	%r3;							\
	bc	4,17,99f;	/* branch if PSL_PR is false */		\
/* interrupts are recoverable here, and enable translation */		\
	RESTORE_KERN_SRS(%r3,%r4);					\
	CPU601_KERN_ENTRY(%r3,%r4);					\
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
	lwz	%r12,IFRAME_R12(%r1)
	lwz	%r11,IFRAME_R11(%r1)
	lwz	%r10,IFRAME_R10(%r1)
	lwz	%r9,IFRAME_R9(%r1)
	lwz	%r8,IFRAME_R8(%r1)
	lwz	%r7,IFRAME_R7(%r1)
	lwz	%r6,IFRAME_SRR1(%r1)
	lwz	%r5,IFRAME_SRR0(%r1)
	lwz	%r4,IFRAME_CTR(%r1)
	lwz	%r3,IFRAME_XER(%r1)
	mtsrr1	%r6
	mtsrr0	%r5
	mtctr	%r4
	mtxer	%r3

	GET_CPUINFO(%r5)
	lwz	%r4,CI_INTRDEPTH(%r5)
	addi	%r4,%r4,-1		/* adjust reentrancy count */
	stw	%r4,CI_INTRDEPTH(%r5)

/* Returning to user mode? */
	mtcr	%r6			/* saved SRR1 */
	bc	4,17,1f			/* branch if PSL_PR is false */

	CPU601_KERN_LEAVE(%r3,%r4)
	RESTORE_USER_SRS(%r3,%r4)
	lwz	%r3,CI_ASTPENDING(%r5)	/* Test AST pending */
	andi.	%r3,%r3,1
	beq	1f

/* Setup for entry to realtrap: */
	lwz	%r3,IFRAME_R1(%r1)	/* get saved SP */
	mtsprg1	%r3
	li	%r6,EXC_AST
	stmw	%r28,CI_TEMPSAVE(%r5)	/* establish tempsave again */
	mtlr	%r6
	lwz	%r28,IFRAME_LR(%r1)	/* saved LR */
	lwz	%r29,IFRAME_CR(%r1)	/* saved CR */
	lwz	%r6,IFRAME_R6(%r1)
	lwz	%r5,IFRAME_R5(%r1)
	lwz	%r4,IFRAME_R4(%r1)
	lwz	%r3,IFRAME_R3(%r1)
	lwz	%r0,IFRAME_R0(%r1)
	b	realtrap
1:
/* Here is the normal exit of extintr: */
	lwz	%r5,IFRAME_CR(%r1)
	lwz	%r6,IFRAME_LR(%r1)
	mtcr	%r5
	mtlr	%r6
	lwz	%r6,IFRAME_R6(%r1)
	lwz	%r5,IFRAME_R5(%r1)
	lwz	%r4,IFRAME_R4(%r1)
	lwz	%r3,IFRAME_R3(%r1)
	lwz	%r0,IFRAME_R0(%r1)
	lwz	%r1,IFRAME_R1(%r1)
	rfi

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
	stmw	%r28,CI_DDBSAVE(%r3)
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
	lwz	%r3,(FRAME_EXC+(2*SZREG))(%r1)	/* save exception */
	GET_CPUINFO(%r4)
	stw	%r3,(CI_DDBSAVE+(2*SZREG))(%r4)
	FRAME_LEAVE(CI_DDBSAVE)
	mtsprg1	%r1			/* prepare for entrance to realtrap */
	GET_CPUINFO(%r1)
	stmw	%r28,CI_TEMPSAVE(%r1)
	mflr	%r28
	mfcr	%r29
	lwz	%r31,(CI_DDBSAVE+(2*SZREG))(%r1)
	mtlr	%r31
	mfsprg1	%r1
	b	realtrap
ddbleave:
	FRAME_LEAVE(CI_DDBSAVE)
	rfi
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
	stmw	%r28,CI_IPKDBSAVE(%r3)
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
	lwz	%r3,(FRAME_EXC+(2*SZREG))(%r1)	/* save exception */
	GET_CPUINFO(%r4)
	stw	%r3,(CI_IPKDBSAVE+(2*SZREG))(%r4)
	FRAME_LEAVE(CI_IPKDBSAVE)
	mtsprg1	%r1			/* prepare for entrance to realtrap */
	GET_CPUINFO(%r1)
	stmw	%r28,CI_TEMPSAVE(%r1)
	mflr	%r28
	mfcr	%r29
	lwz	%r31,(CI_IPKDBSAVE+(2*SZREG))(%r1)
	mtlr	%r31
	mfsprg1	%r1
	b	realtrap
ipkdbleave:
	FRAME_LEAVE(CI_IPKDBSAVE)
	rfi

ipkdbfault:
	ba	_ipkdbfault
_ipkdbfault:
	mfsrr0	%r3
	addi	%r3,%r3,4
	mtsrr0	%r3
	li	%r3,-1
	rfi

/*
 * int ipkdbfbyte(unsigned char *p)
 */
	.globl	_C_LABEL(ipkdbfbyte)
_C_LABEL(ipkdbfbyte):
	li	%r9,EXC_DSI		/* establish new fault routine */
	lwz	%r5,0(%r9)
	lis	%r6,ipkdbfault@ha
	lwz	%r6,ipkdbfault@l(%r6)
	stw	%r6,0(%r9)
#ifdef	IPKDBUSERHACK
	lis	%r8,_C_LABEL(ipkdbsr)@ha
	lwz	%r8,_C_LABEL(ipkdbsr)@l(%r8)
	mtsr	USER_SR,8
	isync
#endif
	dcbst	%r0,%r9			/* flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	lbz	%r3,0(%r3)		/* fetch data */
	stw	%r5,0(%r9)		/* restore previous fault handler */
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
	lwz	%r5,0(%r9)
	lis	%r6,ipkdbfault@ha
	lwz	%r6,ipkdbfault@l(%r6)
	stw	%r6,0(%r9)
#ifdef	IPKDBUSERHACK
	lis	%r8,_C_LABEL(ipkdbsr)@ha
	lwz	%r8,_C_LABEL(ipkdbsr)@l(%r8)
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
	stw	%r5,0(%r9)		/* restore previous fault handler */
	dcbst	%r0,%r9			/* and flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	blr
#endif	/* IPKDB */
	.globl	_C_LABEL(trapend)
_C_LABEL(trapend):
