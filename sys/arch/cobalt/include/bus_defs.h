/*	$NetBSD: bus_defs.h,v 1.1.26.1 2014/08/10 06:53:53 tls Exp $	*/

/*
 * Copyright (c) 1996, 1997, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#ifndef _COBALT_BUS_DEFS_H_
#define _COBALT_BUS_DEFS_H_

/*
 * Bus address and size types
 */
typedef paddr_t bus_addr_t;
typedef psize_t bus_size_t;
#define	PRIxBUSADDR	PRIxPADDR
#define	PRIxBUSSIZE	PRIxPSIZE

#include <mips/locore.h>

/*
 * Access methods for bus resources and address space.
 */
typedef int	bus_space_tag_t;
typedef u_long	bus_space_handle_t;

#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
#define BUS_SPACE_MAP_PREFETCHABLE	0x04

#define __BUS_SPACE_HAS_STREAM_METHODS

#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

#define _MIPS_NEED_BUS_DMA_BOUNCE
#include <mips/bus_dma_defs.h>

#endif /* _COBALT_BUS_DEFS_H_ */
