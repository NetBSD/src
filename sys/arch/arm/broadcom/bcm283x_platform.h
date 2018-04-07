/*	$NetBSD: bcm283x_platform.h,v 1.1.2.2 2018/04/07 04:12:11 pgoyette Exp $	*/

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

#ifndef	_ARM_BCM2835REG_PLATFORM_H_
#define	_ARM_BCM2835REG_PLATFORM_H_

#include <arch/evbarm/fdt/platform.h>

#define	BCM2835_IOPHYSTOVIRT(a) \
	((KERNEL_IO_VBASE | (((a) & 0xf0000000) >> 4)) + ((a) & ~0xff000000))

#define	BCM2835_PERIPHERALS_VBASE \
	BCM2835_IOPHYSTOVIRT(BCM2835_PERIPHERALS_BASE)

#define	BCM2836_PERIPHERALS_VBASE \
	BCM2835_IOPHYSTOVIRT(BCM2836_PERIPHERALS_BASE)

#define	BCM2836_ARM_LOCAL_VBASE \
	BCM2835_IOPHYSTOVIRT(BCM2836_ARM_LOCAL_BASE)

#endif /* _ARM_BCM2835REG_PLATFORM_H_ */
