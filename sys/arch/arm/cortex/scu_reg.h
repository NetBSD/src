/* $NetBSD: scu_reg.h,v 1.1.2.1 2017/12/03 11:35:52 jdolecek Exp $ */
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _ARM_CORTEX_SCUREG_H_
#define _ARM_CORTEX_SCUREG_H_

/*
 * ARM Snoop Control Unit Definitions
 * Used by Cortex-A5 and Cortex-A9
 */

#define	SCU_SIZE		0x100

#define	SCU_CTL			0x00	// SCU Control Register
#define	SCU_CFG			0x04	// SCU Configuration Register
#define	SCU_CPU_PWR_STS		0x08	// SCU CPU Power Status
#define	SCU_INV_ALL_REG		0x0c	// SCU Invalidate All Registers in Secure State
#define	SCU_FILTER_START	0x40	// Filtering Start Address
#define	SCU_FILTER_END		0x44	// Filtering End Address
#define	SCU_ACCESS_CONTROL	0x50	// SCU Access Control
#define	SCU_NS_ACCESS_CONTROL	0x54	// SCU Non-Secure Access Control

#define	SCU_CTL_IC_STANDBY_ENA			__BIT(6)
#define	SCU_CTL_SCU_STANDBY_ENA			__BIT(5)
#define	SCU_CTL_FORCE_PORT0_ENA			__BIT(4)
#define	SCU_CTL_SPECULATIVE_LINEFILL_ENA	__BIT(3)
#define	SCU_CTL_SCU_RAM_PARITY_ENA		__BIT(2)
#define	SCU_CTL_ADDR_FILTER_ENA			__BIT(1)
#define	SCU_CTL_SCU_ENA				__BIT(0)

#define	SCU_CFG_TAG_RAM_SIZE_CPUn(n)	__BITS(9+2*(n),8+2*(n))
#define	SCU_CFG_TAG_RAM_SIZE_CPU3	__BITS(15,14)
#define	SCU_CFG_TAG_RAM_SIZE_CPU2	__BITS(13,12)
#define	SCU_CFG_TAG_RAM_SIZE_CPU1	__BITS(11,10)
#define	SCU_CFG_TAG_RAM_SIZE_CPU0	__BITS(9,8)
#define	SCU_CFG_TAG_RAM_SIZE_16KB	0
#define	SCU_CFG_TAG_RAM_SIZE_32KB	1
#define	SCU_CFG_TAG_RAM_SIZE_64KB	2
#define	SCU_CFG_CPUn_SMP(n)		__BIT(4+(n))
#define	SCU_CFG_CPU3_SMP		__BIT(7)
#define	SCU_CFG_CPU2_SMP		__BIT(6)
#define	SCU_CFG_CPU1_SMP		__BIT(5)
#define	SCU_CFG_CPU0_SMP		__BIT(4)
#define	SCU_CFG_CPUMAX			__BITS(0,1)	// # of CPU - 1

#endif /* _ARM_CORTEX_SCUREG_H_ */
