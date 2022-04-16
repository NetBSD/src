/*	$NetBSD: bus_defs.h,v 1.3 2022/04/16 18:15:21 andvar Exp $	*/

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

#ifndef _SGIMIPS_BUS_DEFS_H_
#define _SGIMIPS_BUS_DEFS_H_

#include <mips/locore.h>

/*
 * Values for sgimips bus space tag, not to be used directly by MI code.
 * XXX these need special handling 
 */

#include <mips/bus_space_defs.h>

extern bus_space_tag_t normal_memt;	/* normal, non-spaced */
extern bus_space_tag_t mace_isa_memt;	/* spaced, for MACE 'ISA' */
extern bus_space_tag_t mace_pci_memt;	/* address-twiddled to look little */
extern bus_space_tag_t mace_pci_iot;	/* endian with <32bit accesses */
extern bus_space_tag_t gio_pci_memt;	/* same, without windows */
extern bus_space_tag_t hpc_memt;	/* 8bit regs at 32bit spacing */

#include <mips/bus_dma_defs.h>
#define	SGIMIPS_DMAMAP_COHERENT _BUS_DMAMAP_COHERENT


#endif /* _SGIMIPS_BUS_DEFS_H_ */
