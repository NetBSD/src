/*	$NetBSD: beccvar.h,v 1.3 2003/03/25 19:47:30 thorpej Exp $	*/

/*
 * Copyright (c) 2002, 2003 Wasabi Systems, Inc.
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

#ifndef _BECCVAR_H_
#define	_BECCVAR_H_

#include <sys/queue.h>
#include <dev/pci/pcivar.h>

/*
 * There are roughly 32 interrupt sources.
 */
#define	NIRQ		32

struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_func)(void *);		/* handler */
	void *ih_arg;			/* arg for handler */
	int ih_ipl;			/* IPL_* */
	int ih_irq;			/* IRQ number */
};

struct intrq {
	TAILQ_HEAD(, intrhand) iq_list;	/* handler list */
	struct evcnt iq_ev;		/* event counter */
	int iq_mask;			/* IRQs to mask while handling */
	int iq_levels;			/* IPL_*'s this IRQ has */
	int iq_ist;			/* share type */
};

struct becc_softc {
	struct device sc_dev;		/* generic device glue */

	/*
	 * We expect the board-specific front-end to have already mapped
	 * the PCI I/O, memory, and configuration spaces.
	 */
	vaddr_t sc_pci_io_base;		/* I/O window vaddr */
	vaddr_t sc_pci_mem_base[2];	/* MEM window vaddr */
	vaddr_t sc_pci_cfg_base;	/* CFG window vaddr */

	/*
	 * These define the 2 32M PCI Inbound windows and 1 128M (rev8 & up).
	 */
	struct {
		uint32_t iwin_base;	/* PCI address */
		uint32_t iwin_xlate;	/* local address */
	} sc_iwin[3];

	/*
	 * Variables that define the 2 32M PCI Outbound windows and
	 * 1 1G (rev8 & up).
	 */
	uint32_t sc_owin_xlate[3];	/* PCI address */

	/*
	 * This is the PCI address that the Outbound I/O
	 * window maps to.
	 */
	uint32_t sc_ioout_xlate;

	/* Bus space, DMA, and PCI tags for the PCI bus. */
	struct bus_space sc_pci_iot;
	struct bus_space sc_pci_memt;
	struct arm32_bus_dma_tag sc_pci_dmat;
	struct arm32_pci_chipset sc_pci_chipset;

	/* DMA window info for PCI DMA. */
	struct arm32_dma_range sc_pci_dma_range[3];

	/* DMA tag for local DMA. */
	struct arm32_bus_dma_tag sc_local_dmat;
};

struct becc_attach_args {
	bus_dma_tag_t	ba_dmat;
};

extern int becc_rev;	/* Set by early bootstrap code */
extern const char *becc_revisions[];
extern void (*becc_hardclock_hook)(void);

void	becc_calibrate_delay(void);

void	becc_icu_init(void);
void	becc_intr_init(void);
void	*becc_intr_establish(int, int, int (*)(void *), void *);
void	becc_intr_disestablish(void *);

void	becc_io_bs_init(bus_space_tag_t, void *);
void	becc_mem_bs_init(bus_space_tag_t, void *);

void	becc_pci_init(pci_chipset_tag_t, void *);

void	becc_attach(struct becc_softc *);

uint32_t becc_pcicore_read(struct becc_softc *, bus_addr_t);
void	becc_pcicore_write(struct becc_softc *, bus_addr_t, uint32_t);

#endif /* _BECCVAR_H_ */
