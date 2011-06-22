/*	$NetBSD: mainbus.h,v 1.5 2011/06/22 18:06:32 matt Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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

#ifndef _PMPPC_DEV_MAINBUS_H_
#define _PMPPC_DEV_MAINBUS_H_

#include <dev/pci/pcivar.h>
#include <machine/pci_machdep.h>

struct mainbus_attach_args {
	const char *mb_name;
	u_long mb_addr;
	int mb_irq;
	bus_space_tag_t mb_bt;		/* Bus space tag */
	bus_dma_tag_t mb_dmat;		/* DMA tag */

	union {
		struct {
			u_int size;
			u_int width;
		} mb_flash;
	} u;
};

extern struct powerpc_bus_space pmppc_mem_tag;
extern struct powerpc_bus_space pmppc_pci_io_tag;
extern struct powerpc_bus_dma_tag pci_bus_dma_tag;

int pmppc_pci_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
void pmppc_pci_conf_interrupt(void *, int, int, int, int, int *);
void pmppc_pci_get_chipset_tag(pci_chipset_tag_t);

#endif /* _PMPPC_DEV_MAINBUS_H_ */
