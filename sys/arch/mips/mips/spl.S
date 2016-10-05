/*	$NetBSD: spl.S,v 1.5.32.3 2016/10/05 20:55:32 skrll Exp $	*/

/*-
 * Copyright (c) 2009, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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

#include "opt_multiprocessor.h"	/* MP kernel? */
#include "opt_cputype.h"	/* which mips CPU levels do we support? */
#include "opt_ddb.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>

RCSID("$NetBSD: spl.S,v 1.5.32.3 2016/10/05 20:55:32 skrll Exp $")

#include "assym.h"

	.data
	.globl	_C_LABEL(ipl_sr_map)
	.type	_C_LABEL(ipl_sr_map),@object
	.p2align INT_SCALESHIFT
_C_LABEL(ipl_sr_map):
	.word	0			/* IPL_NONE */
	.word	MIPS_SOFT_INT_MASK_0	/* IPL_SOFT{CLOCK,BIO} */
	.word	MIPS_SOFT_INT_MASK	/* IPL_SOFT{NET,SERIAL} */
	.word	MIPS_INT_MASK		/* IPL_VM */
	.word	MIPS_INT_MASK		/* IPL_SCHED */
	.word	MIPS_INT_MASK		/* IPL_DDB */
	.word	MIPS_INT_MASK		/* IPL_HIGH */

	.text
	.set	noreorder
/*
 * MIPS processor interrupt control
 *
 * Used as building blocks for spl(9) kernel interface.
 */
_splraise:
	/*
	 * a0 = SR bits to be cleared for this IPL
	 * a1 = this IPL (IPL_*)
	 * Can only use a0-a3 and v0-v1
	 */
	PTR_L	a3, L_CPU(MIPS_CURLWP)
	NOP_L					# load delay
	INT_L	v0, CPU_INFO_CPL(a3)		# get current IPL from cpu_info
	NOP_L					# load delay
	sltu	v1, a1, v0			# newipl < curipl
	bnez	v1, 1f				# yes, don't change.
	 nop					#  branch delay
	mfc0	v1, MIPS_COP_0_STATUS		# fetch status register
	NOP_L					# load delay
	or	v1, MIPS_INT_MASK		# enable all interrupts
	xor	a0, v1				# disable ipl's masked bits
	DYNAMIC_STATUS_MASK(a0,v0)		# machine dependent masking
#if !defined(__mips_o32) && defined(MULTIPROCESSOR)
	li	v1, MIPS3_SR_KX			# keep 64-bit addressing on
	mtc0	v1, MIPS_COP_0_STATUS		## disable interrupts
#else
	mtc0	zero, MIPS_COP_0_STATUS		## disable interrupts
#endif
	COP0_SYNC
#ifdef MULTIPROCESSOR
	PTR_L	a3, L_CPU(MIPS_CURLWP)		## make sure curcpu is correct
	NOP_L					## load delay
#endif
	INT_S	a1, CPU_INFO_CPL(a3)		## save IPL in cpu_info
	mtc0	a0, MIPS_COP_0_STATUS		## store back
	COP0_SYNC
#ifdef PARANOIA
	jr	ra
	 nop					#  branch delay
#endif /* PARANOIA */
1:
#ifdef PARANOIA
	mfc0	v1, MIPS_COP_0_STATUS
	NOP_L					# load delay
	and	a0, v1				# a1 contains bit that MBZ
3:	bnez	a0, 3b				# loop forever
	 nop					#  branch delay
#endif /* PARANOIA */
	jr	ra
	 nop					#  branch delay

STATIC_LEAF(_splsw_splx)
STATIC_XLEAF(_splsw_splx_noprof)		# does not get mcount hooks
#ifdef PARANOIA
	sltiu	v0, a0, IPL_HIGH+1		# v0 = a0 <= IPL_HIGH
98:	beqz	v0, 98b
	 nop
#endif
	PTR_L	a3, L_CPU(MIPS_CURLWP)		# get cpu_info
	NOP_L					# load delay
	INT_L	a2, CPU_INFO_CPL(a3)		# get IPL from cpu_info
	NOP_L					# load delay
	beq	a0, a2, 2f			# if same, nothing to do
	 nop					#  branch delay
#ifdef PARANOIA
	sltu	v0, a0, a2			# v0 = a0 < a2
99:	beqz	v0, 99b				# loop forever if false
	 nop					#  branch delay
#endif /* PARANOIA */
	PTR_LA	v1, _C_LABEL(ipl_sr_map)	# get address of table
	sll	a2, a0, INT_SCALESHIFT		# convert IPL to array offset
	PTR_ADDU v1, a2				# add to table addr
	INT_L	a1, (v1)			# load SR bits for this IPL
1:
	mfc0	v1, MIPS_COP_0_STATUS		# fetch status register
	NOP_L					# load delay
	xor	a1, MIPS_INT_MASK		# invert SR bits
	or	v1, a1				# set any bits for this IPL
	DYNAMIC_STATUS_MASK(v1,t0)		# machine dependent masking
	mtc0	zero, MIPS_COP_0_STATUS		## disable interrupts
	COP0_SYNC
	INT_S	a0, CPU_INFO_CPL(a3)		## save IPL in cpu_info (KSEG0)
	mtc0	v1, MIPS_COP_0_STATUS		## store back
	COP0_SYNC
#ifdef PARANOIA
	jr	ra
	 nop					#  branch delay
#endif /* PARANOIA */
2:
#ifdef PARANOIA
	PTR_LA	v1, _C_LABEL(ipl_sr_map)	# get address of table
	sll	a2, a0, INT_SCALESHIFT		# convert IPL to array offset
	PTR_ADDU v1, a2				# add to table addr
	INT_L	a1, (v1)			# load SR bits for this IPL
	mfc0	v1, MIPS_COP_0_STATUS
	and	v1, MIPS_INT_MASK
	xor	a1, MIPS_INT_MASK
3:	bne	a1, v1, 3b
	 nop					#  branch delay
#endif /* PARANOIA */
	jr	ra
	 nop					#  branch delay
END(_splsw_splx)

STATIC_LEAF(_splsw_spl0)
	INT_L	v1, _C_LABEL(ipl_sr_map) + 4*IPL_NONE
	PTR_L	a3, L_CPU(MIPS_CURLWP)
	or	v1, MIPS_SR_INT_IE		# mask sure interrupts are on
	xor	v1, MIPS_INT_MASK		# invert
	mfc0	a0, MIPS_COP_0_STATUS
	NOP_L					# load delay
	or	v0, a0, v1
	DYNAMIC_STATUS_MASK(v0,t0)		# machine dependent masking
	mtc0	zero, MIPS_COP_0_STATUS		## disable interrupts
	COP0_SYNC
#if IPL_NONE == 0
	INT_S	zero, CPU_INFO_CPL(a3)		## set ipl to 0
#else
#error IPL_NONE != 0
#endif
	mtc0	v0, MIPS_COP_0_STATUS		## enable all sources
	JR_HB_RA				# return (clear hazards)
END(_splsw_spl0)

STATIC_LEAF(_splsw_setsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
	COP0_SYNC
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	NOP_L					# load delay
	or	v0, v0, a0			# set soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	COP0_SYNC
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	JR_HB_RA				# return (clear hazards)
END(_splsw_setsoftintr)

STATIC_LEAF(_splsw_clrsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
	COP0_SYNC
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	nor	a0, zero, a0			# bitwise inverse of A0
	and	v0, v0, a0			# clear soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	COP0_SYNC
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	JR_HB_RA				# return (clear hazards)
END(_splsw_clrsoftintr)

STATIC_LEAF(_splsw_splraise)
#if defined(DDB) && __mips >= 32
	tgeiu	a0, IPL_HIGH+1
#endif
	move	a1, a0
	PTR_LA	v1, _C_LABEL(ipl_sr_map)
	sll	a2, a0, INT_SCALESHIFT
	PTR_ADDU v1, a2
	b	_splraise
	 INT_L	a0, (v1)
END(_splsw_splraise)

STATIC_LEAF(_splsw_splhigh)
STATIC_XLEAF(_splsw_splhigh_noprof)
	PTR_L	a3, L_CPU(MIPS_CURLWP)
	NOP_L					# load delay
	INT_L	v0, CPU_INFO_CPL(a3)		# get current IPL from cpu_info
	li	a1, IPL_HIGH			#
	beq	v0, a1, 1f			# don't do anything if IPL_HIGH
	 nop					# branch delay
	mfc0	v1, MIPS_COP_0_STATUS		# fetch status register
	NOP_L					# load delay
	and	a0, v1, MIPS_INT_MASK		# select all interrupts
	xor	a0, v1				# clear all interrupts
	DYNAMIC_STATUS_MASK(a0,a2)		# machine dependent masking
	mtc0	a0, MIPS_COP_0_STATUS		## store back
	COP0_SYNC
#ifdef MULTIPROCESSOR
	PTR_L	a3, L_CPU(MIPS_CURLWP)		## make sure curcpu is correct
	NOP_L					## load delay
#endif
	INT_S	a1, CPU_INFO_CPL(a3)		## save IPL in cpu_info
#ifdef PARANOIA
	jr	ra				## return
	 nop					#  branch delay
#endif /* PARANOIA */
1:
#ifdef PARANOIA
	mfc0	v1, MIPS_COP_0_STATUS		# fetch status register
	and	v1, MIPS_INT_MASK		# any int bits set?
2:	bnez	v1, 2b				# loop forever.
	 nop					# branch delay
#endif /* PARANOIA */
	jr	ra				## return
	 nop					# branch delay
END(_splsw_splhigh)

	.p2align 4
STATIC_LEAF(_splsw_splddb)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_DDB
	b	_splraise
	 li	a1, IPL_DDB
	nop
END(_splsw_splddb)

STATIC_LEAF(_splsw_splsched)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SCHED
	b	_splraise
	 li	a1, IPL_SCHED
	nop
END(_splsw_splsched)

STATIC_LEAF(_splsw_splvm)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_VM
	b	_splraise
	 li	a1, IPL_VM
	nop
END(_splsw_splvm)

STATIC_LEAF(_splsw_splsoftserial)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SOFTSERIAL
	b	_splraise
	 li	a1, IPL_SOFTSERIAL
	nop
END(_splsw_splsoftserial)

STATIC_LEAF(_splsw_splsoftnet)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SOFTNET
	b	_splraise
	 li	a1, IPL_SOFTNET
	nop
END(_splsw_splsoftnet)

STATIC_LEAF(_splsw_splsoftbio)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SOFTBIO
	b	_splraise
	 li	a1, IPL_SOFTBIO
	nop
END(_splsw_splsoftbio)

STATIC_LEAF(_splsw_splsoftclock)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SOFTCLOCK
	b	_splraise
	 li	a1, IPL_SOFTCLOCK
	nop
END(_splsw_splsoftclock)

STATIC_LEAF(_splsw_splintr)
	mfc0	ta1, MIPS_COP_0_CAUSE		# get active interrupts
	NOP_L					# load delay
						# restrict to hard int bits
	and	v1, ta1, MIPS_HARD_INT_MASK	# now have pending interrupts
	li	v0, IPL_NONE			#  return IPL_NONE
	beq	v1, zero, 2f			# quick exit if nothing pending
	 nop					#  branch delay

	li	v0, IPL_VM			# start at IPL_VM
	PTR_LA	ta3, _C_LABEL(ipl_sr_map) + 4*IPL_VM
	INT_L	ta2, -4(ta3)			# load mask for IPL_SOFTSERIAL
	NOP_L					# load delay
	xor	ta2, MIPS_INT_MASK		# invert
	and	v1, ta2				# apply to pending bits

1:
	INT_L	ta2, (ta3)			# get SR bits for ipl in v0
	NOP_L					# load delay
	xor	ta2, MIPS_INT_MASK		# invert
	and	ta2, v1				# any match to pending intrs?
	beq	ta2, zero, 2f			#  no, return ipl
	 nop					#  branch delay

	PTR_ADDU ta3, 1 << INT_SCALESHIFT	# point to next entry
	addiu	v0, 1				#  increase ipl by 1
	b	1b				# and check it
	 move	v1, ta2				# whittle down pending intrs

2:
	jr	ra
	 INT_S	v1, (a0)			# return a new pending mask
END(_splsw_splintr)

STATIC_LEAF(_splsw_splcheck)
#ifdef PARANOIA
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	NOP_L					# load delay
	INT_L	t1, CPU_INFO_CPL(t0)		# get current priority level

	mfc0	t0, MIPS_COP_0_STATUS		# get current status
	and	t0, MIPS_INT_MASK		# just want INT bits

	PTR_LA	t2, _C_LABEL(ipl_sr_map)
	sll	t1, INT_SCALESHIFT		# shift cpl to array index
	PTR_ADDU t2, t1
	INT_L	t3, (t2)			# load value
	NOP_L					# load delay
	xor	t3, MIPS_INT_MASK		# invert
1:	bne	t0, t3, 1b			# loop forever if not equal
	 nop					#  branch delay
#endif /* PARANOIA */
	jr	ra
	 nop					#  branch delay
END(_splsw_splcheck)

	.rdata
	.globl _C_LABEL(std_splsw)
_C_LABEL(std_splsw):
        PTR_WORD _C_LABEL(_splsw_splhigh)
        PTR_WORD _C_LABEL(_splsw_splsched)
        PTR_WORD _C_LABEL(_splsw_splvm)
        PTR_WORD _C_LABEL(_splsw_splsoftserial)
        PTR_WORD _C_LABEL(_splsw_splsoftnet)
        PTR_WORD _C_LABEL(_splsw_splsoftbio)
        PTR_WORD _C_LABEL(_splsw_splsoftclock)
        PTR_WORD _C_LABEL(_splsw_splraise)
        PTR_WORD _C_LABEL(_splsw_spl0)
        PTR_WORD _C_LABEL(_splsw_splx)
        PTR_WORD _C_LABEL(_splsw_splhigh_noprof)
        PTR_WORD _C_LABEL(_splsw_splx_noprof)
	PTR_WORD _C_LABEL(_splsw_setsoftintr)
	PTR_WORD _C_LABEL(_splsw_clrsoftintr)
	PTR_WORD _C_LABEL(_splsw_splintr)
	PTR_WORD _C_LABEL(_splsw_splcheck)
