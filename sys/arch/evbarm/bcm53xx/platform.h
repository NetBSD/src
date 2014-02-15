/*	$NetBSD: platform.h,v 1.2.16.2 2014/02/15 16:18:37 matt Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#ifndef _EVBARM_BCM53XX_PLATFORM_H
#define _EVBARM_BCM53XX_PLATFORM_H

#include <arm/broadcom/bcm53xx_reg.h>

/*
 * Memory will be mapped starting at 0x8000_0000 through 0xbfff_ffff
 * Kernel VM space: KERNEL_VM_BASE to 0xc0000000.  Leave 1MB unused.
 */
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x40000000)
#if BCM53XX_IO_SIZE >= L1_SS_SIZE
#define	KERNEL_VM_TOP		((0xfff00000 - BCM53XX_IO_SIZE) & -L1_SS_SIZE)
#else
#define	KERNEL_VM_TOP		(0xfff00000 - BCM53XX_IO_SIZE)
#endif
#define	KERNEL_VM_SIZE		(KERNEL_VM_TOP - KERNEL_VM_BASE)

/*
 * BCM53xx ARM Peripherals.  Their physical address would live in the user
 * address space, so we can't map 1:1 VA:PA.  So shove them just after the
 * top of the kernel VM.
 */

#define	KERNEL_IO_VBASE			KERNEL_VM_TOP

#define	KERNEL_IO_PCIE0_OWIN_VBASE	KERNEL_IO_VBASE
#define	KERNEL_IO_PCIE1_OWIN_VBASE	(KERNEL_IO_PCIE0_OWIN_VBASE + BCM53XX_PCIE0_OWIN_SIZE)
#define	KERNEL_IO_PCIE2_OWIN_VBASE	(KERNEL_IO_PCIE1_OWIN_VBASE + BCM53XX_PCIE1_OWIN_SIZE)
#define	KERNEL_IO_IOREG_VBASE		(KERNEL_IO_PCIE2_OWIN_VBASE + BCM53XX_PCIE2_OWIN_SIZE)
#define	KERNEL_IO_ARMCORE_VBASE		(KERNEL_IO_IOREG_VBASE + BCM53XX_IOREG_SIZE)
#define	KERNEL_IO_END_VBASE		(KERNEL_IO_ARMCORE_VBASE + BCM53XX_ARMCORE_SIZE)

#endif	/* _EVBARM_BCM53XX_PLATFORM_H */
