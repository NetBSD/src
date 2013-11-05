/*      $NetBSD: rmixl_pcievar.h,v 1.1.2.10 2013/11/05 18:43:31 matt Exp $	*/
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors.
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

#ifndef _MIPS_RMI_PCIE_VAR_H_
#define _MIPS_RMI_PCIE_VAR_H_

#include <dev/pci/pcivar.h>

typedef enum rmixl_pcie_lnkcfg_mode {
	LCFG_NO=0,		/* placeholder */
	LCFG_EP,		/* end point */
	LCFG_RC,		/* root complex */
} rmixl_pcie_lnkcfg_mode_t;

typedef struct {
	rmixlp_variant_t lnk_variant;
	uint8_t lnk_ports;
	uint8_t lnk_plc_mask;
	uint8_t lnk_plc_match;
	uint8_t lnk_lanes[4];
} rmixlp_pcie_lnkcfg_t;

typedef struct rmixl_pcie_lnkcfg {
	rmixl_pcie_lnkcfg_mode_t mode;
	u_int lanes;
} rmixl_pcie_lnkcfg_t;

typedef struct rmixl_pcie_lnktab {
	u_int ncfgs;
	const char *str;
	const rmixl_pcie_lnkcfg_t *cfg;
} rmixl_pcie_lnktab_t;

typedef struct rmixl_pcie_evcnt {
	struct evcnt evcnt;
	char name[32];
} rmixl_pcie_evcnt_t;

typedef struct rmixl_pcie_link_dispatch {
	rmixl_intrhand_common_t pld_common;
	u_int pld_link;
	u_int pld_bitno;
	u_int pld_irq;
	rmixl_pcie_evcnt_t *pld_evs;	/* index by cpu */
} rmixl_pcie_link_dispatch_t;
#define pld_func	pld_common.ihc_func
#define pld_arg		pld_common.ihc_arg
#ifdef MULTIPROCESSOR
#define pld_mpsafe	pld_common.ihc_mpsafe
#endif

struct rmixl_pcie_softc;

typedef struct rmixl_pcie_link_intr {
	struct rmixl_pcie_softc *pli_sc;
	pcitag_t pli_tag;
	u_int pli_link;
	u_int pli_ipl;
	u_int pli_nplds;
	u_int pli_maxplds;
	void *pli_ih;			/* mips interrupt handle */
	callout_t pli_callout;		/* for delayed free of this struct */
	rmixl_pcie_link_dispatch_t *pli_plds[0];
					/* variable length */
} rmixl_pcie_link_intr_t;

#define RMIXL_PCIE_NLINKS_MAX	4

#define RMIXL_PCIE_EVCNT(sc, link, bitno, cpu)	\
		&(sc)->sc_evcnts[link][(bitno) * (ncpu) + (cpu)]

struct rmixl_pcie_softc;

typedef struct rmixl_pcie_softc {
	device_t	 		sc_dev;
	pci_chipset_tag_t		sc_pc;	 /* in rmixl_configuration */
	bus_space_tag_t			sc_pci_cfg_memt;
	bus_space_tag_t			sc_pci_ecfg_memt;
	bus_space_tag_t			sc_pci_ecfg_eb_memt;
	bus_space_tag_t			sc_pci_ecfg_el_memt;
	bus_space_handle_t		sc_pci_cfg_memh;
#ifdef _LP64
	bus_space_handle_t		sc_pci_ecfg_memh;
#endif
#ifdef MIPS64_XLP
	rmixlp_pcie_lnkcfg_t		sc_lnkcfg;
	pcitag_t			sc_mapprobe;
	uint8_t				sc_lnkmode;
	uint64_t			sc_bswapped;
#endif
	bus_dma_tag_t			sc_dmat29;
	bus_dma_tag_t			sc_dmat32;
	bus_dma_tag_t			sc_dmat64;
	rmixl_pcie_lnktab_t		sc_pcie_lnktab;
	kmutex_t			sc_mutex;
	void *				sc_fatal_ih;
	bool				(*sc_link_intr_change)(struct rmixl_pcie_softc *, u_int, u_int, bool);
	rmixl_pcie_evcnt_t *		sc_evcnts[RMIXL_PCIE_NLINKS_MAX];
	rmixl_pcie_link_intr_t *	sc_link_intr[RMIXL_PCIE_NLINKS_MAX];
#ifdef MIPS64_XLP
	rmixl_pcie_link_intr_t *	sc_msi_intr[32];
	pcireg_t			sc_bus0_bar0_sizes[64];
	struct mips_bus_space		sc_pci_eb_memt;
#endif
} rmixl_pcie_softc_t;

pci_intr_handle_t
	rmixl_pcie_make_pih(u_int, u_int, u_int);
void	rmixl_pcie_decompose_pih(pci_intr_handle_t, u_int *, u_int *, u_int *);

int	rmixl_pcie_intr(void *);
int	rmixl_pcie_link_intr(rmixl_pcie_link_intr_t *, uint64_t);

void	rmixl_pcie_link_intr_disestablish(void *, void *);
void *	rmixl_pcie_link_intr_establish(rmixl_pcie_softc_t *, pci_intr_handle_t,
	    int, int (*)(void *), void *);

void	rmixl_physaddr_init_pcie(struct extent *);

#endif  /* _MIPS_RMI_PCIE_VAR_H_ */
