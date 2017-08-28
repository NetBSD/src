/*	$NetBSD: r8a7743-sysc.h,v 1.1.1.1.6.2 2017/08/28 17:53:04 skrll Exp $	*/

/*
 * Copyright (C) 2016 Cogent Embedded Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __DT_BINDINGS_POWER_R8A7743_SYSC_H__
#define __DT_BINDINGS_POWER_R8A7743_SYSC_H__

/*
 * These power domain indices match the numbers of the interrupt bits
 * representing the power areas in the various Interrupt Registers
 * (e.g. SYSCISR, Interrupt Status Register)
 */

#define R8A7743_PD_CA15_CPU0		 0
#define R8A7743_PD_CA15_CPU1		 1
#define R8A7743_PD_CA15_SCU		12
#define R8A7743_PD_SGX			20

/* Always-on power area */
#define R8A7743_PD_ALWAYS_ON		32

#endif /* __DT_BINDINGS_POWER_R8A7743_SYSC_H__ */
