/*      $NetBSD: rmixl_pcievar.h,v 1.1.2.8 2010/09/20 19:42:31 cliff Exp $	*/
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
	int (*func)(void *);
	void *arg;
	u_int link;
	u_int bitno;
	u_int irq;
	rmixl_pcie_evcnt_t *counts;	/* index by cpu */
} rmixl_pcie_link_dispatch_t;

struct rmixl_pcie_softc;

typedef struct rmixl_pcie_link_intr {
	struct rmixl_pcie_softc *sc;
	u_int link;
	u_int ipl;
	void *ih;			/* mips interrupt handle */
	callout_t callout;		/* for delayed free of this struct */
	u_int dispatch_count;
	rmixl_pcie_link_dispatch_t  dispatch_data[1];
					/* variable length */
} rmixl_pcie_link_intr_t;

#define RMIXL_PCIE_NLINKS_MAX	4

typedef struct rmixl_pcie_softc {
	device_t                	sc_dev;
	struct mips_pci_chipset 	sc_pci_chipset;
	bus_space_tag_t              	sc_pci_cfg_memt;
	bus_space_tag_t              	sc_pci_ecfg_memt;
	bus_dma_tag_t			sc_29bit_dmat;
	bus_dma_tag_t			sc_32bit_dmat;
	bus_dma_tag_t			sc_64bit_dmat;
	rmixl_pcie_lnktab_t		sc_pcie_lnktab;
	kmutex_t			sc_mutex;
	int				sc_tmsk;
	void 			       *sc_fatal_ih;
	rmixl_pcie_evcnt_t	       *sc_evcnts[RMIXL_PCIE_NLINKS_MAX];
	rmixl_pcie_link_intr_t	       *sc_link_intr[RMIXL_PCIE_NLINKS_MAX];
} rmixl_pcie_softc_t;


extern void rmixl_physaddr_init_pcie(struct extent *);

#endif  /* _MIPS_RMI_PCIE_VAR_H_ */

