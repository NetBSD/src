/*	$NetBSD: psychovar.h,v 1.1 1999/06/04 13:42:15 mrg Exp $	*/

/*
 * Copyright (c) 1999 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SPARC64_DEV_PSYCHOVAR_H_
#define _SPARC64_DEV_PSYCHOVAR_H_

/* per real PCI bus info */
struct psycho_softc;

struct psycho_pbm {
	/* link to mum */
	struct psycho_softc		*pp_sc;

	/*
	 * note that the sabre really only has one ranges property,
	 * used for both simba a and simba b (but the ranges for
	 * real psychos are the same for PCI A and PCI B anyway).
	 */
	struct psycho_registers		*pp_regs;
	struct psycho_ranges		*pp_range;
	struct psycho_interrupt_map	*pp_intmap;
	struct psycho_interrupt_map_mask pp_intmapmask;

	/* counts of above */
	int				pp_nregs;
	int				pp_nrange;
	int				pp_nintmap;

	/* chipset tag for this instance */
	pci_chipset_tag_t		pp_pc;

	/* our tags */
	bus_space_tag_t			pp_memt;
	bus_space_tag_t			pp_iot;
	bus_dma_tag_t			pp_dmat;
	int				pp_flags;

	/* and pointers into the psycho regs for our bits */
	struct pci_ctl			*pp_pcictl;
	struct strbuf_diag		*pp_sb_diag;
};

/*
 * per-PCI bus on mainbus softc structure; one for sabre, or two
 * per pair of psycho's.
 */
struct psycho_softc {
	struct	device			sc_dev;

	/*
	 * one sabre has two simba's.  psycho's are separately attached,
	 * with the `other' psycho_pbm allocated at the first's attach.
	 */
	struct psycho_pbm		*sc_sabre;
	struct psycho_pbm		*__sc_psycho_this;
	struct psycho_pbm		*__sc_psycho_other;
#define	sc_simba_a	__sc_psycho_this
#define	sc_simba_b	__sc_psycho_other
#define	sc_psycho_this	__sc_psycho_this
#define	sc_psycho_other	__sc_psycho_other

	/*
	 * PSYCHO register.  we record the base physical address of these 
	 * also as it is the base of the entire PSYCHO
	 */
	struct psychoreg		*sc_regs;
	paddr_t				sc_basepaddr;

	/* our tags (from parent) */
	bus_space_tag_t			sc_bustag;
	bus_dma_tag_t			sc_dmatag;	

	/* config space */
	bus_space_tag_t			sc_configtag;
	paddr_t				sc_configaddr;

	int				sc_clockfreq;
	int				sc_node;	/* prom node */
	int				sc_mode;	/* (whatareya?) */
#define	PSYCHO_MODE_SABRE	1	/* i'm a sabre (yob) */
#define	PSYCHO_MODE_PSYCHO_A	2	/* i'm a psycho (w*nker) */
#define	PSYCHO_MODE_PSYCHO_B	3	/* i'm another psycho (w*nker) */

	struct iommu_state		sc_is;
};

/* config space is per-psycho.  mem/io/dma are per-pci bus */
bus_dma_tag_t psycho_alloc_dma_tag __P((struct psycho_pbm *));
bus_space_tag_t psycho_alloc_bus_tag __P((struct psycho_pbm *, int));

#define psycho_alloc_config_tag(pp) psycho_alloc_bus_tag((pp), PCI_CONFIG_BUS_SPACE)
#define psycho_alloc_mem_tag(pp) psycho_alloc_bus_tag((pp), PCI_MEMORY_BUS_SPACE)
#define psycho_alloc_io_tag(pp) psycho_alloc_bus_tag((pp), PCI_IO_BUS_SPACE)

int psycho_intr_map __P((pcitag_t, int, int, pci_intr_handle_t *));

#endif /* _SPARC64_DEV_PSYCHOVAR_H_ */
