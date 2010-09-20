/*      $NetBSD: rmixl_pcixvar.h,v 1.1.2.3 2010/09/20 19:42:31 cliff Exp $	*/
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

#ifndef _MIPS_RMI_PCIX_VAR_H_
#define _MIPS_RMI_PCIX_VAR_H_

#include <dev/pci/pcivar.h>

typedef struct rmixl_pcix_evcnt {
	struct evcnt evcnt;
	char name[32];
} rmixl_pcix_evcnt_t;

typedef struct rmixl_pcix_dispatch {
	int (*func)(void *);
	void *arg;
	u_int bitno;
	u_int irq;
	rmixl_pcix_evcnt_t *counts;	/* index by cpu */
} rmixl_pcix_dispatch_t;

struct rmixl_pcix_softc;

typedef struct rmixl_pcix_intr {
	struct rmixl_pcix_softc *sc;
	u_int intrpin;
	u_int ipl;
	void *ih;			/* mips interrupt handle */
	callout_t callout;		/* for delayed free of this struct */
	u_int intenb;			/* enabled flags for INT[ABCD] */
	u_int dispatch_count;
	rmixl_pcix_dispatch_t dispatch_data[1];
					/* variable length */
} rmixl_pcix_intr_t;

#define RMIXL_PCIX_NINTR	4	/* PCI INT[A,B,C,D] */

typedef struct rmixl_pcix_softc {
	device_t                	sc_dev;
	struct mips_pci_chipset 	sc_pci_chipset;
	bus_space_tag_t              	sc_pci_cfg_memt;
	bus_space_tag_t              	sc_pci_ecfg_memt;
	bus_dma_tag_t			sc_29bit_dmat;
	bus_dma_tag_t			sc_32bit_dmat;
	bus_dma_tag_t			sc_64bit_dmat;
	kmutex_t			sc_mutex;
	int				sc_tmsk;
	void 			       *sc_fatal_ih;
        rmixl_pcix_evcnt_t             *sc_evcnts;
	rmixl_pcix_intr_t	       *sc_intr;
} rmixl_pcix_softc_t;


extern void rmixl_physaddr_init_pcix(struct extent *);

#endif  /* _MIPS_RMI_PCIX_VAR_H_ */

