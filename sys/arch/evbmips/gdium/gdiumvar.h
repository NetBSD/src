/*	$NetBSD: gdiumvar.h,v 1.1.2.2 2009/08/19 18:46:13 yamt Exp $	*/

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

#include <mips/bonito/bonitovar.h>

struct gdium_config {
	struct bonito_config gc_bonito;

	struct mips_bus_space gc_iot;
	struct mips_bus_space gc_memt;

	struct mips_bus_dma_tag gc_pci_dmat;

	struct mips_pci_chipset gc_pc;

	struct extent *gc_io_ex;
	struct extent *gc_mem_ex;

	int	gc_mallocsafe;
};

struct mainbus_attach_args {
	const char *maa_name;
};

#define	GDIUM_IRQ_GPIO0		0
#define	GDIUM_IRQ_GPIO1		1
#define	GDIUM_IRQ_GPIO2		2
#define	GDIUM_IRQ_GPIO3		3
#define	GDIUM_IRQ_PCI_INTA	4
#define	GDIUM_IRQ_PCI_INTB	5
#define	GDIUM_IRQ_PCI_INTC	6
#define	GDIUM_IRQ_PCI_INTD	7
#define	GDIUM_IRQ_PCI_PERR	8
#define	GDIUM_IRQ_PCI_SERR	9 
#define	GDIUM_IRQ_DENALI	10
#define	GDIUM_IRQ_INT0		11
#define	GDIUM_IRQ_INT1		12
#define	GDIUM_IRQ_INT2		13
#define	GDIUM_IRQ_INT3		14
#define	GDIUM_IRQ_MAX		14

#ifdef _KERNEL
extern struct gdium_config gdium_configuration;

int	gdium_cnattach(struct gdium_config *);

void	gdium_bus_io_init(bus_space_tag_t, void *);
void	gdium_bus_mem_init(bus_space_tag_t, void *);

void	gdium_dma_init(struct gdium_config *);

void	gdium_intr_init(struct gdium_config *);

//void	gdium_iointr(uint32_t, uint32_t, register_t, uint32_t);

void	gdium_cal_timer(bus_space_tag_t, bus_space_handle_t);
#endif /* _KERNEL */
