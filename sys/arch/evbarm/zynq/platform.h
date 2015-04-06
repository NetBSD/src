/*	$NetBSD: platform.h,v 1.1.2.2 2015/04/06 15:17:56 skrll Exp $	*/
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

#ifndef _EVBARM_ZYNQ_PLATFORM_H
#define _EVBARM_ZYNQ_PLATFORM_H

#include <arm/zynq/zynq7000_reg.h>

/*
 * Memory will be mapped starting at 0x8000_0000 through 0xbfff_ffff
 * Kernel VM space: KERNEL_VM_BASE to 0xc0000000.  Leave 1MB unused.
 */
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x40000000)
#define	KERNEL_VM_TOP		((0xfff00000 - ZYNQ7000_IO_SIZE) & -L1_SS_SIZE)
#define	KERNEL_VM_SIZE		(KERNEL_VM_TOP - KERNEL_VM_BASE)

/*
 * Zynq ARM Peripherals.  Their physical address would live in the user
 * address space, so we can't map 1:1 VA:PA.  So shove them just after the
 * top of the kernel VM.
 */

#define	KERNEL_IO_VBASE			KERNEL_VM_TOP

#define	KERNEL_IO_IOREG_VBASE		KERNEL_IO_VBASE
#define	KERNEL_IO_ARMCORE_VBASE		(KERNEL_IO_IOREG_VBASE + ZYNQ7000_IOREG_SIZE)
#define	KERNEL_IO_END_VBASE		(KERNEL_IO_ARMCORE_VBASE + ZYNQ7000_ARMCORE_SIZE)

#endif	/* _EVBARM_ZYNQ_PLATFORM_H */
