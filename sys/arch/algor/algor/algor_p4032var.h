/*	$NetBSD: algor_p4032var.h,v 1.5.28.1 2011/06/06 09:04:41 jruoho Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <machine/bus.h>
#include <dev/pci/pcivar.h>

struct p4032_config {
	struct algor_bus_space ac_lociot;
	struct algor_bus_space ac_iot;
	struct algor_bus_space ac_memt;

	struct algor_bus_dma_tag ac_pci_dmat;
	struct algor_bus_dma_tag ac_pci_pf_dmat;

	struct algor_pci_chipset ac_pc;

	struct extent *ac_io_ex;
	struct extent *ac_mem_ex;

	int	ac_mallocsafe;
};

#ifdef _KERNEL
/*
 * These are indexes into the interrupt mapping table for the
 * on-board devices.
 */
#define	P4032_IRQ_PCICTLR	4
#define	P4032_IRQ_FLOPPY	5
#define	P4032_IRQ_PCKBC		6
#define	P4032_IRQ_COM1		7
#define	P4032_IRQ_COM2		8
#define	P4032_IRQ_LPT		9
#define	P4032_IRQ_GPIO		10
#define	P4032_IRQ_RTC		11

extern struct p4032_config p4032_configuration;

void	algor_p4032loc_bus_io_init(bus_space_tag_t, void *);
void	algor_p4032_bus_io_init(bus_space_tag_t, void *);
void	algor_p4032_bus_mem_init(bus_space_tag_t, void *);

void	algor_p4032_dma_init(struct p4032_config *);

void	algor_p4032_intr_init(struct p4032_config *);

void	algor_p4032_iointr(int, vaddr_t, uint32_t);

void	algor_p4032_cal_timer(bus_space_tag_t, bus_space_handle_t);
#endif /* _KERNEL */
