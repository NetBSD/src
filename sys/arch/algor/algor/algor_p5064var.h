/*	$NetBSD: algor_p5064var.h,v 1.6.28.1 2011/06/06 09:04:41 jruoho Exp $	*/

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
#include <dev/isa/isavar.h>

struct p5064_config {
	struct algor_bus_space ac_iot;
	struct algor_bus_space ac_memt;

	struct algor_bus_dma_tag ac_pci_dmat;
	struct algor_bus_dma_tag ac_isa_dmat;

	struct algor_pci_chipset ac_pc;
	struct algor_isa_chipset ac_ic;

	struct extent *ac_io_ex;
	struct extent *ac_mem_ex;

	int	ac_mallocsafe;
};

#ifdef _KERNEL
#define	P5064_IRQ_ETHERNET	4
#define	P5064_IRQ_SCSI		5
#define	P5064_IRQ_USB		6

#define	P5064_IRQ_MKBD		7
#define	P5064_IRQ_COM1		8
#define	P5064_IRQ_COM2		9
#define	P5064_IRQ_FLOPPY	10
#define	P5064_IRQ_CENTRONICS	11
#define	P5064_IRQ_RTC		12

#define	P5064_IRQ_ISABRIDGE	13
#define	P5064_IRQ_IDE0		14
#define	P5064_IRQ_IDE1		15

extern struct p5064_config p5064_configuration;
extern const int p5064_isa_to_irqmap[];

void	algor_p5064_bus_io_init(bus_space_tag_t, void *);
void	algor_p5064_bus_mem_init(bus_space_tag_t, void *);

void	algor_p5064_dma_init(struct p5064_config *);

void	algor_p5064_intr_init(struct p5064_config *);

void	algor_p5064_iointr(int, vaddr_t, uint32_t);

void	algor_p5064_cal_timer(bus_space_tag_t, bus_space_handle_t);
#endif /* _KERNEL */
