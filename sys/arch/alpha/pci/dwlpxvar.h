/* $NetBSD: dwlpxvar.h,v 1.8 2001/07/27 00:25:20 thorpej Exp $ */

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

#include <dev/pci/pcivar.h>
#include <sys/extent.h>

#include <alpha/pci/pci_sgmap_pte32.h>

#define	_FSTORE	(EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long))

/*
 * DWLPX configuration.
 */
struct dwlpx_config {
	int				cc_initted;
	struct alpha_bus_space		cc_iot;
	struct alpha_bus_space		cc_memt;
	struct extent *			cc_io_ex;
	struct extent *			cc_d_mem_ex;
	struct extent *			cc_s_mem_ex;
	struct alpha_pci_chipset	cc_pc;
	struct dwlpx_softc *		cc_sc;	/* back pointer */
	long				cc_io_exstorage[_FSTORE];
	long				cc_dmem_exstorage[_FSTORE];
	long				cc_smem_exstorage[_FSTORE];
	unsigned long			cc_sysbase;	/* shorthand */
	struct alpha_bus_dma_tag	cc_dmat_direct;
	struct alpha_bus_dma_tag	cc_dmat_sgmap;
	struct alpha_sgmap		cc_sgmap;
};

struct dwlpx_softc {
	struct device		dwlpx_dev;
	struct dwlpx_config	dwlpx_cc;	/* config info */
	u_int16_t		dwlpx_dtype;	/* Node Type */
	u_int8_t		dwlpx_node;	/* TurboLaser Node */
	u_int8_t		dwlpx_hosenum;	/* Hose Number */
	u_int8_t		dwlpx_nhpc;	/* # of hpcs */
	u_int8_t		dwlpx_sgmapsz;	/* size of SGMAP */
};
#define	DWLPX_NONE	0
#define	DWLPX_SG32K	1
#define	DWLPX_SG64K	2
#define	DWLPX_SG128K	3

void	dwlpx_init __P((struct dwlpx_softc *));
void	dwlpx_pci_init __P((pci_chipset_tag_t, void *));
void	dwlpx_dma_init __P((struct dwlpx_config *));

void	dwlpx_bus_io_init __P((bus_space_tag_t, void *));
void	dwlpx_bus_mem_init __P((bus_space_tag_t, void *));

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
