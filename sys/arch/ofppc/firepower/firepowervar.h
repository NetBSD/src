/*	$NetBSD: firepowervar.h,v 1.1 2001/10/29 22:28:39 thorpej Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <dev/pci/pcivar.h>

/*
 * Firepower system configuration.
 *
 * All of the information that the chipset-specific functions need to
 * do their dirty work (and more!).
 */
struct firepower_config {
	int	c_initted;

	struct ppc_bus_space c_iot, c_memt;
	struct ppc_pci_chipset c_pc;

	struct powerpc_bus_dma_tag c_dmat_pci;
	struct powerpc_bus_dma_tag c_dmat_isa;

	struct extent *c_io_ex, *c_mem_ex;
	int	c_mallocsafe;

	uint32_t c_type;	/* PowerPro or PowerTop */
};

struct firepower_softc {
	struct device sc_dev;

	struct firepower_config *sc_cp;
};

void	firepower_chipset_init(struct firepower_config *, int);
void	firepower_pci_init(pci_chipset_tag_t, void *);
void	firepower_dma_init(struct firepower_config *);

void	firepower_bus_io_init(bus_space_tag_t, void *);
void	firepower_bus_mem_init(bus_space_tag_t, void *);

void	firepower_intr_init(void);
void	firepower_pci_intr_init(pci_chipset_tag_t, void *);
