/* $NetBSD: dwlpxvar.h,v 1.12 2023/12/04 00:32:10 thorpej Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/extent.h>
#include <sys/vmem_impl.h>

#include <dev/pci/pcivar.h>

#include <alpha/pci/pci_sgmap_pte32.h>

#define	DWLPX_IO_NBTS		VMEM_EST_BTCOUNT(2, 8)
#define	DWLPX_D_MEM_NBTS	VMEM_EST_BTCOUNT(1, 8)
#define	DWLPX_S_MEM_NBTS	VMEM_EST_BTCOUNT(2, 8)

/*
 * DWLPX configuration.
 */
struct dwlpx_config {
	int				cc_initted;
	struct alpha_bus_space		cc_iot;
	struct alpha_bus_space		cc_memt;
	vmem_t *			cc_io_arena;
	vmem_t *			cc_d_mem_arena;
	vmem_t *			cc_s_mem_arena;
	struct alpha_pci_chipset	cc_pc;
	struct dwlpx_softc *		cc_sc;	/* back pointer */
	struct vmem			cc_io_arena_store;
	struct vmem			cc_d_mem_arena_store;
	struct vmem			cc_s_mem_arena_store;
	struct vmem_btag		cc_io_btag_store[DWLPX_IO_NBTS];
	struct vmem_btag		cc_d_mem_btag_store[DWLPX_D_MEM_NBTS];
	struct vmem_btag		cc_s_mem_btag_store[DWLPX_S_MEM_NBTS];
	unsigned long			cc_sysbase;	/* shorthand */
	struct alpha_bus_dma_tag	cc_dmat_direct;
	struct alpha_bus_dma_tag	cc_dmat_sgmap;
	struct alpha_sgmap		cc_sgmap;
};

struct dwlpx_softc {
	device_t		dwlpx_dev;
	struct dwlpx_config	dwlpx_cc;	/* config info */
	uint16_t		dwlpx_dtype;	/* Node Type */
	uint8_t		dwlpx_node;	/* TurboLaser Node */
	uint8_t		dwlpx_hosenum;	/* Hose Number */
	uint8_t		dwlpx_nhpc;	/* # of hpcs */
	uint8_t		dwlpx_sgmapsz;	/* size of SGMAP */
};
#define	DWLPX_NONE	0
#define	DWLPX_SG32K	1
#define	DWLPX_SG64K	2
#define	DWLPX_SG128K	3

void	dwlpx_init(struct dwlpx_softc *);
void	dwlpx_pci_init(pci_chipset_tag_t, void *);
void	dwlpx_dma_init(struct dwlpx_config *);

void	dwlpx_bus_io_init(bus_space_tag_t, void *);
void	dwlpx_bus_mem_init(bus_space_tag_t, void *);

/*
 * Each DWLPX supports up to 15 devices, 12 of which are PCI slots.
 *
 * Since the STD I/O modules in slots 12-14 are really a PCI-EISA
 * bridge, we'll punt on those for the moment.
 */
#define	DWLPX_MAXDEV	12

#define	DWLPX_NIONODE	5
#define	DWLPX_NHOSE	4

/*
 * Default values to put into DWLPX IMASK register(s)
 */
#define	DWLPX_IMASK_DFLT	\
	(1 << 24) |	/* IPL 17 for error interrupts */ \
	(1 << 17) |	/* IPL 14 for device interrupts */ \
	(1 << 16)	/* Enable Error Interrupts */
