/* $NetBSD: pl310_reg.h,v 1.1 2012/09/02 16:55:10 matt Exp $ */
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

#ifndef _ARM_CORTEX_PL310_REG_H_
#define _ARM_CORTEX_PL310_REG_H_

/*
 * ARM PL310 L2 Cache Controller
 * Used by Cortex cores
 */

#define	L2C_CACHE_ID		0x000
#define	CACHE_ID_IMPL		__BITS(31,24)
#define	CACHE_ID_ID		__BITS(15,10)
#define	CACHE_ID_PART		__BITS(9,6)
#define	CACHE_ID_PART_PL310	3
#define	CACHE_ID_REV		__BITS(5,0)
#define	CACHE_ID_REV_R3P3	9
#define	CACHE_ID_REV_R3P2	8
#define	L2C_CACHE_TYPE		0x004
#define	CACHE_TYPE_DATA_BANKING	__BIT(31)
#define	CACHE_TYPE_CTYPE	__BITS(28,25)
#define	CACHE_TYPE_HARVARD	__BIT(24)
#define	CACHE_TYPE_DSIZE	__BITS(23,12)
#define	CACHE_TYPE_ISIZE	__BITS(11,0)
#define	CACHE_TYPE_xWAYSIZE	__BITS(10,8)
#define	CACHE_TYPE_xASSOC	__BIT(6)
#define	CACHE_TYPE_xLINESIZE	__BITS(5,0)

#define	L2C_CTL			0x100
#define	L2C_AUXCTL		0x104
#define	L2C_TAGRAM_CTL		0x108
#define	L2C_DATARAM_CTL		0x10c

#define	L2C_EV_CTR_CTL		0x200
#define	L2C_EV_CTR1_CTL		0x204
#define	L2C_EV_CTR0_CTL		0x208
#define	L2C_EV_CTR1		0x20c
#define	L2C_EV_CTR0		0x210
#define	L2C_INT_MASK		0x214
#define	L2C_INT_MASK_STS	0x218
#define	L2C_INT_RAW_STS		0x21c
#define	L2C_INT_CLR		0x220

#define	L2C_CACHE_SYNC		0x730
#define	L2C_INV_PA		0x770
#define	L2C_INV_WAY		0x77c
#define	L2C_CLEAN_PA		0x7b0
#define	L2C_CLEAN_INDEX		0x7b8
#define	L2C_CLEAN_WAY		0x7bc
#define	L2C_CLEAN_INV_PA	0x7f0
#define	L2C_CLEAN_INV_INDEX	0x7f8
#define	L2C_CLEAN_INV_WAY	0x7fc

#define	L2C_D_LOCKDOWN0		0x900
#define	L2C_I_LOCKDOWN0		0x904
#define	L2C_D_LOCKDOWN1		0x908
#define	L2C_I_LOCKDOWN1		0x90c
#define	L2C_D_LOCKDOWN2		0x910
#define	L2C_I_LOCKDOWN2		0x914
#define	L2C_D_LOCKDOWN3		0x918
#define	L2C_I_LOCKDOWN3		0x91c
#define	L2C_D_LOCKDOWN4		0x920
#define	L2C_I_LOCKDOWN4		0x924
#define	L2C_D_LOCKDOWN5		0x928
#define	L2C_I_LOCKDOWN5		0x92c
#define	L2C_D_LOCKDOWN6		0x930
#define	L2C_I_LOCKDOWN6		0x934
#define	L2C_D_LOCKDOWN7		0x938
#define	L2C_I_LOCKDOWN7		0x93c
#define	L2C_LOCK_LINE_EN	0x950
#define	L2C_UNLOCK_WAY		0x954

#define	L2C_ADDR_FILTER_START	0xc00
#define	L2C_ADDR_FILTER_END	0xc04

#define	L2C_DEBUG_CTL		0xf40
#define	L2C_PREFETCH_CTL	0xf60
#define	L2C_POWER_CTL		0xf80

#endif /* _ARM_CORTEX_PL310_REG_H_ */
