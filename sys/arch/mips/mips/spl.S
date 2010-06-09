/*	$NetBSD: spl.S,v 1.1.2.7 2010/06/09 14:25:30 matt Exp $	*/

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

#include "opt_cputype.h"	/* which mips CPU levels do we support? */

#include <sys/cdefs.h>

#include <machine/param.h>
#include <mips/asm.h>
#include <mips/cpuregs.h>

RCSID("$NetBSD: spl.S,v 1.1.2.7 2010/06/09 14:25:30 matt Exp $")

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
	INT_L	v0, CPU_INFO_CPL(a3)		# get current IPL from cpu_info
	sltu	v1, a1, v0			# newipl < curipl
	bnez	v1, 1f				# yes, don't change.
	mfc0	v1, MIPS_COP_0_STATUS		# fetch status register
	or	v1, MIPS_INT_MASK		# enable all interrupts
	xor	a0, v1				# disable ipl's masked bits
	DYNAMIC_STATUS_MASK(a0,v0)		# machine dependent masking
	mtc0	zero, MIPS_COP_0_STATUS		## disable interrupts
	COP0_SYNC
	INT_S	a1, CPU_INFO_CPL(a3)		## save IPL in cpu_info
	mtc0	a0, MIPS_COP_0_STATUS		## store back
	COP0_SYNC
#ifdef PARANOIA
	j	ra
	 nop
#endif /* PARANOIA */
1:
#ifdef PARANOIA
	mfc0	v1, MIPS_COP_0_STATUS
	and	a0, v1				# a1 contains bit that MBZ
3:	bnez	a0, 3b				# loop forever
	 nop
#endif /* PARANOIA */
	j	ra
	 nop

STATIC_LEAF(_splsw_splx)
STATIC_XLEAF(_splsw_splx_noprof)		# does not get mcount hooks
	PTR_L	a3, L_CPU(MIPS_CURLWP)		# get cpu_info
	INT_L	a2, CPU_INFO_CPL(a3)		# get IPL from cpu_info
	beq	a0, a2, 2f			# if same, nothing to do
	 nop
#ifdef PARANOIA
	sltu	v0, a0, a2			# v0 = a0 < a2
99:	beqz	v0, 99b				# loop forever if false
	 nop
#endif /* PARANOIA */
	#move	a1, zero			# avoid lookup on splx(IPL_NONE)
	#beq	a0, zero, 1f			# skip fetch
	PTR_LA	v1, _C_LABEL(ipl_sr_map)	# get address of table
	sll	a2, a0, INT_SCALESHIFT		# convert IPL to array offset
	PTR_ADDU v1, a2				# add to table addr
	INT_L	a1, (v1)			# load SR bits for this IPL
1:
	mfc0	v1, MIPS_COP_0_STATUS		# fetch status register
	or	v1, MIPS_INT_MASK		# set all INT bits
	xor	v1, a1				# clear any bits for this IPL
	DYNAMIC_STATUS_MASK(v1,t0)		# machine dependent masking
	mtc0	zero, MIPS_COP_0_STATUS		## disable interrupts
	COP0_SYNC
	INT_S	a0, CPU_INFO_CPL(a3)		## save IPL in cpu_info (KSEG0)
	mtc0	v1, MIPS_COP_0_STATUS		## store back
	COP0_SYNC
#ifdef PARANOIA
	j	ra
	 nop
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
	 nop
#endif /* PARANOIA */
	j	ra
	 nop
END(_splsw_splx)

STATIC_LEAF(_splsw_spl0)
	INT_L	v1, _C_LABEL(ipl_sr_map) + 4*IPL_NONE
	xor	v1, MIPS_INT_MASK | MIPS_SR_INT_IE # invert and or in IE
	PTR_L	a3, L_CPU(MIPS_CURLWP)
	#mtc0	zero, MIPS_COP_0_CAUSE		# clear SOFT_INT bits
	COP0_SYNC
	mfc0	v0, MIPS_COP_0_STATUS
	nop
	nop
	or	v0, v1
	DYNAMIC_STATUS_MASK(v0,t0)		# machine dependent masking
	mtc0	zero, MIPS_COP_0_STATUS		## disable interrupts
	COP0_SYNC
#if IPL_NONE == 0
	INT_S	zero, CPU_INFO_CPL(a3)		## set ipl to 0
#else
#error IPL_NONE != 0
#endif
	mtc0	v0, MIPS_COP_0_STATUS		## enable all sources
	COP0_SYNC
	j	ra
	nop
END(_splsw_spl0)

STATIC_LEAF(_splsw_setsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
	COP0_SYNC
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	nop
	or	v0, v0, a0			# set soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	COP0_SYNC
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	COP0_SYNC
	j	ra
	nop
END(_splsw_setsoftintr)

STATIC_LEAF(_splsw_clrsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
	COP0_SYNC
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	nor	a0, zero, a0			# bitwise inverse of A0
	and	v0, v0, a0			# clear soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	COP0_SYNC
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	COP0_SYNC
	j	ra
	nop
END(_splsw_clrsoftintr)

STATIC_LEAF(_splsw_splraise)
	move	a1, a0
	PTR_LA	v1, _C_LABEL(ipl_sr_map)
	sll	a2, a0, INT_SCALESHIFT
	PTR_ADDU v1, a2
	INT_L	a0, (v1)
	b	_splraise
	 nop
END(_splsw_splraise)

STATIC_LEAF(_splsw_splhigh)
STATIC_XLEAF(_splsw_splhigh_noprof)
	PTR_L	a3, L_CPU(MIPS_CURLWP)
	INT_L	v0, CPU_INFO_CPL(a3)		# get current IPL from cpu_info
	li	a1, IPL_HIGH			# 
	beq	v0, a1, 1f			# don't do anything if IPL_HIGH
	mfc0	v1, MIPS_COP_0_STATUS		# fetch status register
	and	a0, v1, MIPS_INT_MASK		# select all interrupts
	xor	a0, v1				# clear all interrupts
	DYNAMIC_STATUS_MASK(a0,a2)		# machine dependent masking
	mtc0	a0, MIPS_COP_0_STATUS		## store back
	COP0_SYNC
	INT_S	a1, CPU_INFO_CPL(a3)		## save IPL in cpu_info
#ifdef PARANOIA
	j	ra				## return
	 nop
#endif /* PARANOIA */
1:
#ifdef PARANOIA
	mfc0	v1, MIPS_COP_0_STATUS		# fetch status register
	and	v1, MIPS_INT_MASK		# any int bits set?
2:	bnez	v1, 2b				# loop forever.
	 nop
#endif /* PARANOIA */
	j	ra				## return
	 nop
END(_splsw_splhigh)

STATIC_LEAF(_splsw_splddb)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_DDB
	li	a1, IPL_DDB
	b	_splraise
	 nop
END(_splsw_splddb)

STATIC_LEAF(_splsw_splsched)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SCHED
	li	a1, IPL_SCHED
	b	_splraise
	 nop
END(_splsw_splsched)

STATIC_LEAF(_splsw_splvm)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_VM
	li	a1, IPL_VM
	b	_splraise
END(_splsw_splvm)

STATIC_LEAF(_splsw_splsoftserial)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SOFTSERIAL
	li	a1, IPL_SOFTSERIAL
	b	_splraise
	 nop
END(_splsw_splsoftserial)

STATIC_LEAF(_splsw_splsoftnet)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SOFTNET
	li	a1, IPL_SOFTNET
	b	_splraise
	 nop
END(_splsw_splsoftnet)

STATIC_LEAF(_splsw_splsoftbio)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SOFTBIO
	li	a1, IPL_SOFTBIO
	b	_splraise
	 nop
END(_splsw_splsoftbio)

STATIC_LEAF(_splsw_splsoftclock)
	INT_L	a0, _C_LABEL(ipl_sr_map) + 4*IPL_SOFTCLOCK
	li	a1, IPL_SOFTCLOCK
	b	_splraise
	 nop
END(_splsw_splsoftclock)

STATIC_LEAF(_splsw_splintr)
	mfc0	ta1, MIPS_COP_0_CAUSE		# get active interrupts

						# restrict to hard int bits
	and	v1, ta1, MIPS_HARD_INT_MASK	# now have pending interrupts
	li	v0, IPL_NONE			#  return IPL_NONE
	beq	v1, zero, 2f			# quick exit if nothing pending
	 nop

	li	v0, IPL_VM			# start at IPL_VM
	PTR_LA	ta3, _C_LABEL(ipl_sr_map) + 4*IPL_VM
	INT_L	ta2, -4(ta3)			# load mask for IPL_SOFTSERIAL
	xor	ta2, MIPS_INT_MASK		# invert
	and	v1, ta2				# apply to pending bits

1:
	INT_L	ta2, (ta3)			# get SR bits for ipl in v0
	xor	ta2, MIPS_INT_MASK		# invert
	and	ta2, v1				# any match to pending intrs?
	beq	ta2, zero, 2f			#  no, return ipl

	PTR_ADDU ta3, 1 << INT_SCALESHIFT	# point to next entry
	addiu	v0, 1				#  increase ipl by 1
	move	v1, ta2				# whittle down pending intrs
	b	1b				# and check it

2:
	INT_S	v1, (a0)			# return a new pending mask
	j	ra
	#nop
END(_splsw_splintr)

STATIC_LEAF(_splsw_splcheck)
#ifdef PARANOIA
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	INT_L	t1, CPU_INFO_CPL(t0)		# get current priority level

	mfc0	t0, MIPS_COP_0_STATUS		# get current status
	and	t0, MIPS_INT_MASK		# just want INT bits

	PTR_LA	t2, _C_LABEL(ipl_sr_map)
	sll	t1, INT_SCALESHIFT		# shift cpl to array index
	PTR_ADDU t2, t1
	INT_L	t3, (t2)			# load value
	xor	t3, MIPS_INT_MASK		# invert
1:	bne	t0, t3, 1b			# loop forever if not equal
	 nop
#endif /* PARANOIA */
	j	ra
	 nop
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
