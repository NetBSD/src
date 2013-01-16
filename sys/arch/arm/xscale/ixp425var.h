/*	$NetBSD: ixp425var.h,v 1.13.2.2 2013/01/16 05:32:50 yamt Exp $ */

/*
 * Copyright (c) 2003
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

#ifndef _IXP425VAR_H_
#define _IXP425VAR_H_

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <sys/bus.h>

#include <dev/pci/pcivar.h>

#define	PCI_CSR_WRITE_4(sc, reg, data)	\
	bus_space_write_4(sc->sc_iot, sc->sc_pci_ioh,	\
		reg, data)

#define	PCI_CSR_READ_4(sc, reg)	\
	bus_space_read_4(sc->sc_iot, sc->sc_pci_ioh, reg)

#define	GPIO_CONF_WRITE_4(sc, reg, data)	\
	bus_space_write_4(sc->sc_iot, sc->sc_gpio_ioh,  \
		reg, data)

#define	GPIO_CONF_READ_4(sc, reg) \
	bus_space_read_4(sc->sc_iot, sc->sc_gpio_ioh, reg)

#define	EXP_BUS_WRITE_4(sc, reg, data)	\
	bus_space_write_4(sc->sc_iot, sc->sc_exp_ioh, reg, data)

#define	EXP_BUS_READ_4(sc, reg) \
	bus_space_read_4(sc->sc_iot, sc->sc_exp_ioh, reg)

#define PCI_CONF_LOCK(s)	(s) = disable_interrupts(I32_bit)
#define PCI_CONF_UNLOCK(s)	restore_interrupts((s))

struct ixp425_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;		/* IRQ handle */

	uint32_t sc_intrmask;

	/* Handles for the various subregions. */
	bus_space_handle_t sc_pci_ioh;		/* PCI mem handler */
	bus_space_handle_t sc_gpio_ioh;		/* GPIOs handler */
	bus_space_handle_t sc_exp_ioh;		/* Expansion bus handle */

	/* Bus space, DMA, and PCI tags for the PCI bus */
	struct bus_space sc_pci_iot;
	struct bus_space sc_pci_memt;
	struct arm32_bus_dma_tag ia_pci_dmat;
	struct arm32_pci_chipset ia_pci_chipset;
	vaddr_t sc_pci_va;

	/* DMA window info for PCI DMA. */
	struct arm32_dma_range ia_pci_dma_range;

	/* GPIO configuration */
	uint32_t sc_gpio_out;
	uint32_t sc_gpio_oe;
	uint32_t sc_gpio_intr1;
	uint32_t sc_gpio_intr2;
};

/*
 * There are roughly 32 interrupt sources.
 */
#define	NIRQ		32

struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_func)(void *);		/* interrupt handler */
	void *ih_arg;			/* arg for handler */
	int ih_ipl;			/* IPL_* */
	int ih_irq;			/* IRQ number */
};

#define	IRQNAMESIZE	sizeof("ixp425 irq xx")

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

extern struct ixp425_softc *ixp425_softc;

extern struct bus_space ixpsip_bs_tag;
extern struct bus_space ixp425_bs_tag;
extern struct bus_space ixp425_a4x_bs_tag;

void	ixp425_bs_init(bus_space_tag_t, void *);
void	ixp425_md_pci_init(struct ixp425_softc *);
void	ixp425_md_pci_conf_interrupt(pci_chipset_tag_t, int, int, int,
	    int, int *);
void	ixp425_pci_init(struct ixp425_softc *);
void	ixp425_pci_dma_init(struct ixp425_softc *);
void	ixp425_io_bs_init(bus_space_tag_t, void *);
void	ixp425_mem_bs_init(bus_space_tag_t, void *);

void	ixp425_pci_conf_reg_write(struct ixp425_softc *, uint32_t, uint32_t);
uint32_t ixp425_pci_conf_reg_read(struct ixp425_softc *, uint32_t); 

void	ixp425_attach(device_t);
void	ixp425_icu_init(void);
void	ixp425_clk_bootstrap(bus_space_tag_t);
void	ixp425_intr_init(void);
void	ixp425_intr_evcnt_attach(void);
void	*ixp425_intr_establish(int, int, int (*)(void *), void *);
void    ixp425_intr_disestablish(void *);

uint32_t ixp425_sdram_size(void);

#endif /* _IXP425VAR_H_ */
