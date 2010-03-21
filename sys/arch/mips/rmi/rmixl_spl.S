/*	$NetBSD: rmixl_spl.S,v 1.1.2.1 2010/03/21 21:17:01 cliff Exp $	*/

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

RCSID("$NetBSD: rmixl_spl.S,v 1.1.2.1 2010/03/21 21:17:01 cliff Exp $");

#include "assym.h"

#define PARANOIA	/* XXX TMP FIXME */

#define MAP_SCALESHIFT		3
#define RMIXL_SOFT_INT_MASK_0	(MIPS_SOFT_INT_MASK_1 >> 8)
#define RMIXL_SOFT_INT_MASK	(MIPS_SOFT_INT_MASK >> 8)
#define RMIXL_INT_MASK_1	(MIPS_INT_MASK_1 >> 8)
#define RMIXL_INT_MASK_5	(MIPS_INT_MASK_5 >> 8)
#define RMIXL_COP_0_EIRR	_(9), 6
#define RMIXL_COP_0_EIMR	_(9), 7


/* 
 * Array of mask of bits to set in the EIMR when we go to a
 * given hardware interrupt priority level.
 */
	.rdata
	.globl	_C_LABEL(ipl_eimr_map)
	.type	_C_LABEL(ipl_eimr_map),@object
	.p2align MAP_SCALESHIFT
_C_LABEL(ipl_eimr_map):
	.dword	~0			/* IPL_NONE */
	.dword	~RMIXL_SOFT_INT_MASK_0	/* IPL_SOFT{CLOCK,BIO} */
	.dword	~RMIXL_SOFT_INT_MASK	/* IPL_SOFT{NET,SERIAL} */
	.dword	RMIXL_INT_MASK_5	/* IPL_VM */
	.dword	0			/* IPL_SCHED */
#if IPL_SCHED != IPL_HIGH
	.dword	0			/* IPL_HIGH */
#endif

	.text
/*
 * RMIXL processor interrupt control
 *
 * Used as building blocks for spl(9) kernel interface.
 */
_splraise:
	/*
	 * a0 = EIMR bits to be set for this IPL
	 * a1 = this IPL (IPL_*)
	 * Can only use a0-a3 and v0-v1
	 */
	PTR_L	a3, L_CPU(MIPS_CURLWP)
	INT_L	v0, CPU_INFO_CPL(a3)		# get current IPL from cpu_info
	sltu	v1, a1, v0			# newipl < curipl
	bnez	v1, 2f				# yes, don't change.
	 mfc0	a2, MIPS_COP_0_STATUS		# load STATUS
	and	a2, ~MIPS_INT_MASK		# clear STATUS[IM]
	sll	v1, a0, 8			# EIMR[7:0] to STATUS[15:8]
	and	v1, MIPS_INT_MASK		#  "        "   "
	or	v1, a2				# new STATUS value
	mtc0	zero, MIPS_COP_0_STATUS		## disable all ints in STATUS
	INT_S	a1, CPU_INFO_CPL(a3)		## save IPL in cpu_info
	dmtc0	a0, RMIXL_COP_0_EIMR		## set new EIMR
	mtc0	v1, MIPS_COP_0_STATUS		## set new STATUS
#ifdef PARANOIA
	j	ra
	 nop
#endif /* PARANOIA */
#ifdef PARANOIA
	dmfc0	v0, RMIXL_COP_0_EIMR		# get EIMR
1:	bne	a0, v0, 1b			# loop forever if not equal
	 nop
#endif /* PARANOIA */
2:	j	ra
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
	#beq	a0, zero, 1f			# skip load
	PTR_LA	v1, _C_LABEL(ipl_eimr_map)	# get address of table
	sll	a2, a0, MAP_SCALESHIFT		# convert IPL to array offset
	PTR_ADDU v1, a2				# add to table addr
	REG_L	v1, (v1)			# load EIMR bits for this IPL
1:
	dmfc0	a2, MIPS_COP_0_STATUS		# load STATUS
	and	a2, ~MIPS_INT_MASK		# clear STATUS[IM]
	sll	v0, v1, 8			# EIMR[7:0] to STATUS[15:8]
	and	v0, MIPS_INT_MASK		#  "        "   "
	or	v0, a2				# new STATUS value
	dmtc0	zero, RMIXL_COP_0_EIMR		## disable all interrupts
	INT_S	a0, CPU_INFO_CPL(a3)		## save IPL in cpu_info (KSEG0)
	dmtc0	v1, RMIXL_COP_0_EIMR		## set new EIMR
	mtc0	v0, MIPS_COP_0_STATUS		## set new STATUS
#ifdef PARANOIA
	j	ra
	 nop
#endif /* PARANOIA */
2:
#ifdef PARANOIA
	PTR_LA	v1, _C_LABEL(ipl_eimr_map)	# get address of table
	sll	a2, a0, MAP_SCALESHIFT		# convert IPL to array offset
	PTR_ADDU v1, a2				# add to table addr
	REG_L	a1, (v1)			# load EIMR bits for this IPL
	dmfc0	v1, RMIXL_COP_0_EIMR		# get EIMR
3:	bne	a1, v1, 3b			# loop forever if not equal
	 nop
#endif /* PARANOIA */
	j	ra
	 nop
END(_splsw_splx)

STATIC_LEAF(_splsw_spl0)
	REG_L	v1, _C_LABEL(ipl_eimr_map) + 8*IPL_NONE
	PTR_L	a3, L_CPU(MIPS_CURLWP)
	mtc0	zero, MIPS_COP_0_CAUSE		# clear SOFT_INT bits
	dmfc0	a2, MIPS_COP_0_STATUS		# load STATUS
	and	a2, ~MIPS_INT_MASK		# clear STATUS[IM]
	sll	v0, v1, 8			# EIMR[7:0] to STATUS[15:8]
	and	v0, MIPS_INT_MASK		#  "        "   "
	or	v0, MIPS_SR_INT_IE		# set STATUS[IE]
	or	v0, a2				# new STATUS value
	mtc0	zero, MIPS_COP_0_STATUS		## disable all interrupts
#if IPL_NONE == 0
	INT_S	zero, CPU_INFO_CPL(a3)		## set ipl to 0
#else
#error IPL_NONE != 0
#endif
	dmtc0	v1, RMIXL_COP_0_EIMR		## set new EIMR
	mtc0	v0, MIPS_COP_0_STATUS		## set new STATUS
	nop
	nop
	j	ra
	 nop
END(_splsw_spl0)

LEAF_NOPROFILE(rmixl_spln)
	PTR_LA	v1, _C_LABEL(ipl_eimr_map)	# get address of table
	sll	a2, a0, MAP_SCALESHIFT		# convert IPL to array offset
	PTR_ADDU v1, a2				# add to table addr
	REG_L	v0, (v1)			# load EIMR bits for this IPL
	j	ra
	 nop
END(rmixl_spln)

STATIC_LEAF(_splsw_setsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		## disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE		## load cause register
	nop
	or	v0, v0, a0			## set soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		## store back
	mtc0	v1, MIPS_COP_0_STATUS		## enable interrupts
	j	ra
	nop
END(_splsw_setsoftintr)

STATIC_LEAF(_splsw_clrsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		## disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE		## load cause register
	nor	a0, zero, a0			## bitwise inverse of A0
	and	v0, v0, a0			## clear soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		## store back
	mtc0	v1, MIPS_COP_0_STATUS		## enable interrupts
	j	ra
	nop
END(_splsw_clrsoftintr)

STATIC_LEAF(_splsw_splraise)
	move	a1, a0
	PTR_LA	v1, _C_LABEL(ipl_eimr_map)
	sll	a2, a0, MAP_SCALESHIFT
	PTR_ADDU v1, a2
	REG_L	a0, (v1)
	b	_splraise
	 nop
END(_splsw_splraise)

STATIC_LEAF(_splsw_splhigh)
STATIC_XLEAF(_splsw_splhigh_noprof)
	PTR_L	a3, L_CPU(MIPS_CURLWP)
	INT_L	v0, CPU_INFO_CPL(a3)		# get current IPL from cpu_info
	li	a1, IPL_HIGH			# 
	beq	v0, a1, 1f			# don't do anything if IPL_HIGH
	mfc0	v1, MIPS_COP_0_STATUS		# load STATUS
	move	a2, zero			# clear for EIMR
	and	a0, v1, MIPS_INT_MASK		# select all interrupts
	xor	a0, v1				# clear STATUS[IM]
	mtc0	zero, MIPS_COP_0_STATUS		## disable all interrupts
	INT_S	a1, CPU_INFO_CPL(a3)		## save IPL in cpu_info
	dmtc0	a2, RMIXL_COP_0_EIMR		## set new EIMR
	mtc0	a0, MIPS_COP_0_STATUS		## set new STATUS
	 nop					# XXXXX
#ifdef PARANOIA
	j	ra				# return
	 nop
#endif /* PARANOIA */
1:
#ifdef PARANOIA
	dmfc0	v1, RMIXL_COP_0_EIMR		# load EIMR
2:	bnez	v1, 2b				# loop forever if not 0.
	 nop
#endif /* PARANOIA */
	j	ra				## return
	 nop
END(_splsw_splhigh)

STATIC_LEAF(_splsw_splsched)
	REG_L	a0, _C_LABEL(ipl_eimr_map) + 8*IPL_SCHED
	li	a1, IPL_SCHED
	b	_splraise
	 nop
END(_splsw_splsched)

STATIC_LEAF(_splsw_splvm)
	REG_L	a0, _C_LABEL(ipl_eimr_map) + 8*IPL_VM
	li	a1, IPL_VM
	b	_splraise
END(_splsw_splvm)

STATIC_LEAF(_splsw_splsoftserial)
	REG_L	a0, _C_LABEL(ipl_eimr_map) + 8*IPL_SOFTSERIAL
	li	a1, IPL_SOFTSERIAL
	b	_splraise
	 nop
END(_splsw_splsoftserial)

STATIC_LEAF(_splsw_splsoftnet)
	REG_L	a0, _C_LABEL(ipl_eimr_map) + 8*IPL_SOFTNET
	li	a1, IPL_SOFTNET
	b	_splraise
	 nop
END(_splsw_splsoftnet)

STATIC_LEAF(_splsw_splsoftbio)
	REG_L	a0, _C_LABEL(ipl_eimr_map) + 8*IPL_SOFTBIO
	li	a1, IPL_SOFTBIO
	b	_splraise
	 nop
END(_splsw_splsoftbio)

STATIC_LEAF(_splsw_splsoftclock)
	REG_L	a0, _C_LABEL(ipl_eimr_map) + 8*IPL_SOFTCLOCK
	li	a1, IPL_SOFTCLOCK
	b	_splraise
	 nop
END(_splsw_splsoftclock)

STATIC_LEAF(_splsw_splintr)
	dmfc0	ta1, RMIXL_COP_0_EIRR		# get active interrupts
						# restrict to hard int bits:
	and	v1, ta1, RMIXL_SOFT_INT_MASK	#  v1 &= ~RMIXL_SOFT_INT_MASK
	xor	v1, ta1				#   "      "

	li	v0, IPL_NONE			# return IPL_NONE ...
	beq	v1, zero, 2f			# ... if nothing pending
	 nop

	li	v0, IPL_VM			# start at IPL_VM
	PTR_LA	ta3, _C_LABEL(ipl_eimr_map) + 8*IPL_VM
	REG_L	ta2, -8(ta3)			# load 'enabled' bits for IPL_SOFTSERIAL
						#  ta2 has 'enabled' ints
	and	v1, ta2				# apply to pending bits

1:
	REG_L	ta2, (ta3)			# load 'enabled' bits for ipl in v0
	and	ta2, v1				# any match to pending intrs?
	beq	ta2, zero, 2f			#  no, return ipl

	PTR_ADDU ta3, 1 << MAP_SCALESHIFT	# point to next entry
	addiu	v0, 1				#  increase ipl by 1
	move	v1, ta2				# reduce down pending intrs
	b	1b				# and check them

2:
	/*
	 * Emulate the CP0_SR 'IM' bits in 'pending'
	 * - if clock intr is requested, set MIPS_INT_MASK_5
	 * - if other HW intr is requested, set MIPS_INT_MASK_1 as summary bit
	 *   the RMI evbmips_iointr function will sort through
	 *   individial EIRR requests
	 */
	beq	v1, zero, 4f			# skip ahead if nothing pending
	 li	t2, RMIXL_INT_MASK_5		# load RMIXL_INT_MASK_5
	and	t1, v1, t2			# save count/compare intr request value
	nor	t0, zero, t2			# invert the mask
	and	v1, t0				# v1 &= ~RMIXL_INT_MASK_5
	beq	v1, zero, 3f			# no non-clock intrs? skip ahead
	 li	v1, RMIXL_INT_MASK_1		# use INT_MASK_1 as 'summary' bit
						#  for non-clock hw intrs
3:
	or	v1, t1				# combine clock and non-clock-summary
	sll	v1, MIPS_INT_MASK_SHIFT		# shift to emulate COP0_SR 'IM' bits
4:
	INT_S	v1, (a0)			# set a (fake) new pending mask
	j	ra				# and return highest ipl pending
	 nop
END(_splsw_splintr)

STATIC_LEAF(_splsw_splcheck)
#ifdef PARANOIA
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	INT_L	t1, CPU_INFO_CPL(t0)		# get current priority level

	dmfc0	t0, RMIXL_COP_0_EIMR		# get current EIMR

	PTR_LA	t2, _C_LABEL(ipl_eimr_map)
	sll	t1, MAP_SCALESHIFT		# shift cpl to array index
	PTR_ADDU t2, t1
	REG_L	t3, (t2)			# load value
1:	bne	t0, t3, 1b			# loop forever if not equal
	 nop
#endif /* PARANOIA */
	j	ra
	 nop
END(_splsw_splcheck)

	.rdata
	.globl _C_LABEL(rmixl_splsw)
_C_LABEL(rmixl_splsw):
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
