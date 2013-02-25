/* $Id: imx23_apbdma.h,v 1.1.6.2 2013/02/25 00:28:27 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#ifndef _ARM_IMX_IMX23_APBDMA_H_
#define _ARM_IMX_IMX23_APBDMA_H_

#include <sys/cdefs.h>

/*
 * Public interface to apbdma for device drivers.
 */
void * apbdma_dmamem_alloc(device_t, int, bus_size_t);

/*
 * Common definitions for both APBH and APBX DMA.
 */
#define APBH_DMA_N_CHANNELS	8
#define APBX_DMA_N_CHANNELS	16

#define CMDPIOWORDS_MAX	3

/*
 * DMA command type.
 */
#define NO_DMA_XFER	0
#define DMA_WRITE	1
#define DMA_READ	2
#define DMA_SENSE	3

/*
 * Command word 0.
 */
#define NEXTCMDADDR	__BITS(31, 0)

/*
 * Command word 1.
 */
#define XFER_COUNT	__BITS(31, 16)
#define CMDPIOWORDS	__BITS(15, 12)
#define CMDWORD1_RSVD0	__BITS(11, 8)
#define WAIT4ENDCMD	__BIT(7)
#define SEMAPHORE	__BIT(6)
#define NANDWAIT4READY	__BIT(5)
#define NANDLOCK	__BIT(4)
#define IRQONCMPLT	__BIT(3)
#define CHAIN		__BIT(2)
#define COMMAND		__BITS(1, 0)

/*
 * Command word 2.
 */
#define BUF_ADDR	__BITS(31, 0)

/*
 * Command word 3-n.
 */
#define PIOWORD		__BITS(31, 0)

#endif /* !_ARM_IMX_IMX23_APBDMA_H_ */
