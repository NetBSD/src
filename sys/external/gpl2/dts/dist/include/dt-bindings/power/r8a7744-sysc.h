/*	$NetBSD: r8a7744-sysc.h,v 1.1.1.1.6.2 2019/06/10 22:08:57 christos Exp $	*/

/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018 Renesas Electronics Corp.
 */
#ifndef __DT_BINDINGS_POWER_R8A7744_SYSC_H__
#define __DT_BINDINGS_POWER_R8A7744_SYSC_H__

/*
 * These power domain indices match the numbers of the interrupt bits
 * representing the power areas in the various Interrupt Registers
 * (e.g. SYSCISR, Interrupt Status Register)
 *
 * Note that RZ/G1N is identical to RZ/G2M w.r.t. power domains.
 */

#define R8A7744_PD_CA15_CPU0		 0
#define R8A7744_PD_CA15_CPU1		 1
#define R8A7744_PD_CA15_SCU		12
#define R8A7744_PD_SGX			20

/* Always-on power area */
#define R8A7744_PD_ALWAYS_ON		32

#endif /* __DT_BINDINGS_POWER_R8A7744_SYSC_H__ */
