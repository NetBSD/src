/*	$NetBSD: locore_octeon.S,v 1.1 2015/04/29 08:32:00 hikaru Exp $	*/

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
RCSID("$NetBSD: locore_octeon.S,v 1.1 2015/04/29 08:32:00 hikaru Exp $")

#include "opt_cputype.h"

#include <mips/cpuregs.h>
#include <arch/mips/cavium/dev/octeon_corereg.h>

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
	.set	mips3
	.set	arch=octeon

	.text

LEAF(mips_cp0_cvmctl_read)
	_CP0_READ64(CP0_CVMCTL)
END(mips_cp0_cvmctl_read)

LEAF(mips_cp0_cvmctl_write)
	_CP0_WRITE64(CP0_CVMCTL)
END(mips_cp0_cvmctl_write)

LEAF(mips_cp0_cvmmemctl_read)
	_CP0_READ64(CP0_CVMMEMCTL)
END(mips_cp0_cvmmemctl_read)

LEAF(mips_cp0_cvmmemctl_write)
	_CP0_WRITE64(CP0_CVMMEMCTL)
END(mips_cp0_cvmmemctl_write)

LEAF(mips_cp0_cvmcnt_read)
	_CP0_READ64(CP0_CVMCNT)
END(mips_cp0_cvmcnt_read)

LEAF(mips_cp0_cvmcnt_write)
	_CP0_WRITE64(CP0_CVMCNT)
END(mips_cp0_cvmcnt_write)

LEAF(mips_cp0_ebase_read)
	_CP0_READ64(CP0_EBASE)
END(mips_cp0_ebase_read)

LEAF(mips_cp0_ebase_write)
	_CP0_WRITE64(CP0_EBASE)
END(mips_cp0_ebase_write)

LEAF(mips_cp0_performance_counter_control0_read)
	_CP0_READ64(CP0_PCCTL)
END(mips_cp0_performance_counter_control0_read)

LEAF(mips_cp0_performance_counter_control0_write)
	_CP0_WRITE64(CP0_PCCTL)
END(mips_cp0_performance_counter_control0_write)

LEAF(mips_cp0_performance_counter_control1_read)
	_CP0_READ64(CP0_PCCTL1)
END(mips_cp0_performance_counter_control1_read)

LEAF(mips_cp0_performance_counter_control1_write)
	_CP0_WRITE64(CP0_PCCTL1)
END(mips_cp0_performance_counter_control1_write)

LEAF(mips_cp0_performance_counter_counter0_read)
	_CP0_READ64(CP0_PCCNT)
END(mips_cp0_performance_counter_counter0_read)

LEAF(mips_cp0_performance_counter_counter0_write)
	_CP0_WRITE64(CP0_PCCNT)
END(mips_cp0_performance_counter_counter0_write)

LEAF(mips_cp0_performance_counter_counter1_read)
	_CP0_READ64(CP0_PCCNT1)
END(mips_cp0_performance_counter_counter1_read)

LEAF(mips_cp0_performance_counter_counter1_write)
	_CP0_WRITE64(CP0_PCCNT1)
END(mips_cp0_performance_counter_counter1_write)
