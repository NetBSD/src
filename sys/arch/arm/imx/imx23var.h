/* $Id: imx23var.h,v 1.1.6.3 2014/08/20 00:02:46 tls Exp $ */

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

#ifndef _ARM_IMX_IMX23VAR_H
#define _ARM_IMX_IMX23VAR_H

#define DRAM_BASE 0x40000000
#define APBH_BASE 0x80000000
#define APBH_SIZE 0x00040000	/* 256 kB */
#define APBX_BASE 0x80040000
#define APBX_SIZE 0x00040000
#define AHB_BASE  0x80080000	/* USB and DRAM registers. */
#define AHB_SIZE  0x00080000	/* 512 kB */

#define IMX23_UART_CLK 24000000

#ifndef _LOCORE
#include <machine/bus_defs.h>

extern struct bus_space imx23_bus_space;
extern struct arm32_bus_dma_tag imx23_bus_dma_tag;

struct apb_attach_args {
	bus_space_tag_t aa_iot;
	bus_dma_tag_t aa_dmat;
	const char *aa_name;
	bus_addr_t aa_addr;
	bus_size_t aa_size;
	int8_t aa_irq;
};

struct apb_softc {
	device_t	sc_dev;
	device_t	dmac; /* DMA controller device for this bus. */
};

struct ahb_attach_args {
	bus_space_tag_t aa_iot;
	bus_dma_tag_t aa_dmat;
	const char *aa_name;
	bus_addr_t aa_addr;
	bus_size_t aa_size;
	int8_t aa_irq;
};

struct ahb_softc {
	device_t	sc_dev;
};

#endif /* !_LOCORE */
#endif /* !_ARM_IMX_IMX23VAR_H */
