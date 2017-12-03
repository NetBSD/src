/*	$NetBSD: rpi.h,v 1.1.4.2 2017/12/03 11:36:06 jdolecek Exp $	*/

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

#ifndef _EVBARM_RPI_RPI_H
#define _EVBARM_RPI_RPI_H

#include <arm/broadcom/bcm2835reg.h>

/*
 * Memory may be mapped VA:PA starting at 0x80000000:0x00000000
 * RPI2 has 1GB upto 0xc0000000
 *
 * Kernel VM space: 800MB at KERNEL_VM_BASE
 */
#define	KERNEL_VM_BASE		0xc0000000
#define	KERNEL_VM_SIZE		(BCM2835_PERIPHERALS_VBASE - KERNEL_VM_BASE)

/*
 * BCM2835 ARM Peripherals
 */
#define	RPI_KERNEL_IO_VBASE		BCM2835_PERIPHERALS_VBASE
#define	RPI_KERNEL_IO_PBASE		BCM2835_PERIPHERALS_BASE
#define	RPI_KERNEL_IO_VSIZE		BCM2835_PERIPHERALS_SIZE

/*
 * BCM2836 Local control block
 */
#define	RPI_KERNEL_LOCAL_VBASE	BCM2836_ARM_LOCAL_VBASE
#define	RPI_KERNEL_LOCAL_PBASE	BCM2836_ARM_LOCAL_BASE
#define	RPI_KERNEL_LOCAL_VSIZE	BCM2836_ARM_LOCAL_SIZE

#define	RPI_REF_FREQ		19200000

#endif	/* _EVBARM_RPI_RPI_H */
