/*	$NetBSD: dvma.h,v 1.2.2.2 2001/04/21 17:54:49 bouyer Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Matthew Fredette.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * DVMA (Direct Virtual Memory Access)
 *
 * For the unfamiliar, this is just DMA where the device doing DMA
 * operates in a virtual address space.  The virtual to physical
 * translations are controlled by the same MMU used bu the CPU.
 * Usually, the virtual space accessed by DVMA devices is a small
 * sub-range of the CPU virtual space, and that range is known as 
 * DVMA space.  
 */

#include <machine/idprom.h>

/*
 * Note that while the DVMA harware makes the last 1MB visible
 * for secondary masters, the PROM "owns" the last page of it.
 * XXX fredette - is this because of the obio ie SCP?
 * Also note that OBIO devices can actually see all of
 * of kernel virtual space.
 */
#define DVMA_MAP_BASE		0x00F00000
#define DVMA_MAP_SIZE_120	0x00040000
#define DVMA_MAP_SIZE_50	0x000F8000
#define DVMA_MAP_SIZE		(cpu_machine_id == SUN2_MACH_120 ? DVMA_MAP_SIZE_120 : DVMA_MAP_SIZE_50)
#define DVMA_MAP_AVAIL		(DVMA_MAP_SIZE-NBPG)

/*
 * To convert an address in DVMA space to a slave address,
 * just use a logical AND with one of the following masks.
 * To convert back, just logical OR with the base address.
 */
#define DVMA_OBIO_SLAVE_BASE	0x0F000000
#define DVMA_OBIO_SLAVE_MASK	0x00FFffff	/* 16MB */

#define DVMA_MBMEM_SLAVE_BASE 	0x00F00000
#define DVMA_MBMEM_SLAVE_MASK 	0x000Fffff	/*  1MB */

#define DVMA_VME_SLAVE_BASE 	0x00F00000
#define DVMA_VME_SLAVE_MASK 	0x000Fffff	/*  1MB */

