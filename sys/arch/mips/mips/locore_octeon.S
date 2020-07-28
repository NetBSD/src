/*	$NetBSD: locore_octeon.S,v 1.13 2020/07/28 00:35:38 simonb Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <mips/asm.h>
RCSID("$NetBSD: locore_octeon.S,v 1.13 2020/07/28 00:35:38 simonb Exp $")

#include "cpunode.h"			/* for NWDOG */
#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <mips/cpuregs.h>
#include <arch/mips/cavium/dev/octeon_corereg.h>

RCSID("$NetBSD: locore_octeon.S,v 1.13 2020/07/28 00:35:38 simonb Exp $")

#include "assym.h"

#define	_CP0_READ64(_cp0)				\
	dmfc0	v0, _cp0;				\
	j	ra;					\
	 nop

#define	_CP0_WRITE64(_cp0)				\
	dmtc0	a0, _cp0;				\
	j	ra;					\
	 nop

	.set	noreorder
	.set	noat
	.set	arch=octeon

	.text

LEAF(mips_cp0_cvmctl_read)
	_CP0_READ64(MIPS_COP_0_CVMCTL)
END(mips_cp0_cvmctl_read)

#ifdef notyet /* the rest of these aren't used (yet) */
LEAF(mips_cp0_cvmctl_write)
	_CP0_WRITE64(MIPS_COP_0_CVMCTL)
END(mips_cp0_cvmctl_write)

LEAF(mips_cp0_cvmmemctl_read)
	_CP0_READ64(MIPS_COP_0_CVMMEMCTL)
END(mips_cp0_cvmmemctl_read)

LEAF(mips_cp0_cvmmemctl_write)
	_CP0_WRITE64(MIPS_COP_0_CVMMEMCTL)
END(mips_cp0_cvmmemctl_write)

LEAF(mips_cp0_cvmcnt_read)
	_CP0_READ64(MIPS_COP_0_CVMCNT)
END(mips_cp0_cvmcnt_read)

LEAF(mips_cp0_cvmcnt_write)
	_CP0_WRITE64(MIPS_COP_0_CVMCNT)
END(mips_cp0_cvmcnt_write)

LEAF(mips_cp0_performance_counter_control0_read)
	_CP0_READ64(MIPS_COP_0_PERFCNT0_CTL)
END(mips_cp0_performance_counter_control0_read)

LEAF(mips_cp0_performance_counter_control0_write)
	_CP0_WRITE64(MIPS_COP_0_PERFCNT0_CTL)
END(mips_cp0_performance_counter_control0_write)

LEAF(mips_cp0_performance_counter_control1_read)
	_CP0_READ64(MIPS_COP_0_PERFCNT1_CTL)
END(mips_cp0_performance_counter_control1_read)

LEAF(mips_cp0_performance_counter_control1_write)
	_CP0_WRITE64(MIPS_COP_0_PERFCNT1_CTL)
END(mips_cp0_performance_counter_control1_write)

LEAF(mips_cp0_performance_counter_counter0_read)
	_CP0_READ64(MIPS_COP_0_PERFCNT0_CNT)
END(mips_cp0_performance_counter_counter0_read)

LEAF(mips_cp0_performance_counter_counter0_write)
	_CP0_WRITE64(MIPS_COP_0_PERFCNT0_CNT)
END(mips_cp0_performance_counter_counter0_write)

LEAF(mips_cp0_performance_counter_counter1_read)
	_CP0_READ64(MIPS_COP_0_PERFCNT1_CNT)
END(mips_cp0_performance_counter_counter1_read)

LEAF(mips_cp0_performance_counter_counter1_write)
	_CP0_WRITE64(MIPS_COP_0_PERFCNT1_CNT)
END(mips_cp0_performance_counter_counter1_write)
#endif /* notyet */

#ifdef MULTIPROCESSOR

NESTED_NOPROFILE(octeon_cpu_spinup, 0, ra)
	//
	// Since the OCTEON cpus doesn't a COP0 OSCONTEXT register, each core
	// must has its own exception vector page.  The exceptions will be
	// modified to refer to that CPU's cpu_info structure.
	//
	mfc0	s1, MIPS_COP_0_EBASE	# get EBASE
	andi	s0, s1, MIPS_EBASE_CPUNUM # fetch cpunum
	# insert cpunum as exception address base:
	ins	s1, s0, MIPS_EBASE_EXC_BASE_SHIFT, MIPS_EBASE_CPUNUM_WIDTH
	ehb
	mtc0	s1, MIPS_COP_0_EBASE	# set EBASE
	COP0_SYNC

	// Indicate this CPU was started by u-boot
	PTR_LA	a0, _C_LABEL(cpus_booted)
	li	a1, 1
	jal	_C_LABEL(atomic_or_64)
	 sllv	a1, a1, s0		# shift cpu number to bit position

	// Wait until cpuid_infos[cpunum] is not NULL.
	PTR_LA	a1, _C_LABEL(cpuid_infos)
	dsll	v0, s0, PTR_SCALESHIFT	# cpunum -> array index
	PTR_ADD	t0, a1, v0		# add to array start
1:	sync
	PTR_L	a1, (t0)		# get cpu_info pointer
	beqz	a1, 1b			# loop until non-NULL
	 nop

	j	_C_LABEL(cpu_trampoline)
	 nop
END(octeon_cpu_spinup)
#endif /* MULTIPROCESSOR */

#if NWDOG > 0 || defined(DDB)

#define	UINT64_C(x)	(x)

#include <mips/cavium/dev/octeon_ciureg.h>

NESTED_NOPROFILE(octeon_reset_vector, 0, ra)
	mfc0	k0, MIPS_COP_0_STATUS	# get cp0 status
	bbit1	k0, V_MIPS3_SR_SR, 1f	# MIPS3_SR_SR
	 ins	k0, zero, V_MIPS_SR_BEV, 1 # clear boot exception vectors
	mtc0	k0, MIPS_COP_0_STATUS	# write cp0 status
	ehb				# hazard barrier
#ifdef MULTIPROCESSOR
	mfc0	k0, MIPS_COP_0_EBASE	# get EBASE
	andi	k0, k0, MIPS_EBASE_CPUNUM # fetch cpunum
	dsll	k0, k0, PTR_SCALESHIFT	# cpunum -> array index
	PTR_LA	k1, _C_LABEL(cpuid_infos)
	PTR_ADDU k1, k1, k0		# add to array start
	PTR_L	k0, (k1)		# get cpu_info
#else
	PTR_LA	k0, _C_LABEL(cpu_info_store) # get cpu_info
#endif
	j	_C_LABEL(mips64r2_kern_nonmaskable_intr)
	 sd	zero, CIU_NMI_OFFSET(k1)# clear NMI
1:
	li	k1, ((MIPS_XKPHYS_START|CIU_BASE) >> 32)	# CIU base (MSW)
	dsll	k1, 32			# shift it place
	ld	k0, CIU_FUSE_OFFSET(k1)	# get mask of CPUs
	sd	k0, CIU_SOFT_RST_OFFSET(k1)	# reset them
	ld	v0, CIU_SOFT_RST_OFFSET(k1)	# force a load
	sd	k0, CIU_SOFT_RST_OFFSET(k1)	# do it again.
2:
	wait				# wait forever
	b	2b			# and loop until reset
	 nop
END(octeon_reset_vector)
#endif
