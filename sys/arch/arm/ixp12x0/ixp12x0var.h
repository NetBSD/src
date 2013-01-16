/*	$NetBSD: ixp12x0var.h,v 1.9.2.2 2013/01/16 05:32:49 yamt Exp $ */
/*
 * Copyright (c) 2002
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _IXP12X0VAR_H_
#define _IXP12X0VAR_H_

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <sys/bus.h>

#include <dev/pci/pcivar.h>

struct ixp12x0_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;

	/* Handles for the PCI */
	bus_space_handle_t sc_pci_ioh;		/* PCI CSR */
	bus_space_handle_t sc_conf0_ioh;	/* PCI Configuration 0 */
	bus_space_handle_t sc_conf1_ioh;	/* PCI Configuration 1 */

	/* DMA, and PCI chipset */
        struct arm32_bus_dma_tag ia_pci_dmat;
        struct arm32_pci_chipset ia_pci_chipset;

	/* DMA window info for PCI DMA. */
	struct arm32_dma_range ia_pci_dma_range;

	/* GPIO */
};

struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_func)(void *);		/* interrupt handler */
	void *ih_arg;			/* arg for handler */
	int ih_ipl;			/* IPL_* */
	int ih_irq;			/* IRQ number */
};

#define	IRQNAMESIZE	sizeof("ixpintr ipl xxx")

struct intrq {
	TAILQ_HEAD(, intrhand) iq_list;	/* handler list */
	struct evcnt iq_ev;		/* event counter */
	uint32_t iq_mask;		/* IRQs to mask while handling */
	uint32_t iq_pci_mask;		/* PCI IRQs to mask while handling */
	uint32_t iq_levels;		/* IPL_*'s this IRQ has */
	char iq_name[IRQNAMESIZE];	/* interrupt name */
	int iq_ist;			/* share type */
};

struct pmap_ent {
	const char*	msg;
	vaddr_t		va;
	paddr_t		pa;
	vsize_t		sz;
	int		prot;
	int		cache;
};

extern struct bus_space	ixp12x0_bs_tag;

void	ixp12x0_pci_init(pci_chipset_tag_t, void *);
void	ixp12x0_pci_dma_init(struct ixp12x0_softc *);
void	ixp12x0_attach(struct ixp12x0_softc *);
void	ixp12x0_intr_init(void);
void	*ixp12x0_intr_establish(int irq, int ipl, int (*)(void *), void *);
void	ixp12x0_intr_disestablish(void *);
void	ixp12x0_pmap_chunk_table(vaddr_t l1pt, struct pmap_ent* m);
void	ixp12x0_pmap_io_reg(vaddr_t l1pt);
void	ixp12x0_reset(void);

#endif /* _IXP12X0VAR_H_ */
