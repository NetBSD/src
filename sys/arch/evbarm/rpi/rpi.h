/*	$NetBSD: rpi.h,v 1.2.2.1 2014/08/10 06:53:56 tls Exp $	*/

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
 * Kernel VM space: KERNEL_VM_BASE to 0xf0000000
 */
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x20000000)
#define	KERNEL_VM_SIZE		(0xf0000000 - KERNEL_VM_BASE)

/*
 * BCM2835 ARM Peripherals
 */
#define	RPI_KERNEL_IO_VBASE		BCM2835_PERIPHERALS_VBASE
#define	RPI_KERNEL_IO_PBASE		BCM2835_PERIPHERALS_BASE
#define	RPI_KERNEL_IO_VSIZE		BCM2835_PERIPHERALS_SIZE

#endif	/* _EVBARM_RPI_RPI_H */
