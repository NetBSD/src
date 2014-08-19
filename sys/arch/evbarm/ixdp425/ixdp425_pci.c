/*      $NetBSD: ixdp425_pci.c,v 1.8.12.2 2014/08/20 00:02:55 tls Exp $ */
#define PCI_DEBUG
/*
 * Copyright (c) 2003
 *      Ichiro FUKUHARA <ichiro@ichiro.org>.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixdp425_pci.c,v 1.8.12.2 2014/08/20 00:02:55 tls Exp $");

/*
 * IXDP425 PCI interrupt support.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <sys/bus.h>

#include <evbarm/ixdp425/ixdp425reg.h>
#include <evbarm/ixdp425/ixdp425var.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/ppbreg.h>

static int ixdp425_pci_intr_map(const struct pci_attach_args *,
    pci_intr_handle_t *);
static const char *ixdp425_pci_intr_string(void *, pci_intr_handle_t, char *, size_t);
static const struct evcnt *ixdp425_pci_intr_evcnt(void *, pci_intr_handle_t);
static void *ixdp425_pci_intr_establish(void *, pci_intr_handle_t, int,
	int (*func)(void *), void *);
static void ixdp425_pci_intr_disestablish(void *, void *);

void
ixp425_md_pci_init(struct ixp425_softc *sc)
{
	pci_chipset_tag_t pc = &sc->ia_pci_chipset;
	uint32_t reg;

	/*
	 * PCI initialization
	 */
	pc->pc_intr_v = sc;
	pc->pc_intr_map = ixdp425_pci_intr_map;
	pc->pc_intr_string = ixdp425_pci_intr_string;
	pc->pc_intr_evcnt = ixdp425_pci_intr_evcnt;
	pc->pc_intr_establish = ixdp425_pci_intr_establish;
	pc->pc_intr_disestablish = ixdp425_pci_intr_disestablish;

	/* PCI Reset Assert */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPOUTR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPOUTR, reg & ~(1U << GPIO_PCI_RESET));

	/* PCI Clock Disable */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPCLKR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPCLKR, reg & ~GPCLKR_MUX14);

	/*
	 * set GPIO Direction
	 *	Output: PCI_CLK, PCI_RESET
	 *	Input:  PCI_INTA, PCI_INTB, PCI_INTC, PCI_INTD
	 */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPOER);
	reg &= ~(1U << GPIO_PCI_CLK);
	reg &= ~(1U << GPIO_PCI_RESET);
	reg |= ((1U << GPIO_PCI_INTA) | (1U << GPIO_PCI_INTB) |
		(1U << GPIO_PCI_INTC) | (1U << GPIO_PCI_INTD));
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPOER, reg);

	/* clear ISR */
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPISR,
			  (1U << GPIO_PCI_INTA) | (1U << GPIO_PCI_INTB) |
			  (1U << GPIO_PCI_INTC) | (1U << GPIO_PCI_INTD));

	/* wait 1ms to satisfy "minimum reset assertion time" of the PCI spec */
	DELAY(1000);
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPCLKR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPCLKR, reg |
		(0xf << GPCLKR_CLK0DC_SHIFT) | (0xf << GPCLKR_CLK0TC_SHIFT));

	/* PCI Clock Enable */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPCLKR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPCLKR, reg | GPCLKR_MUX14);

	/*
	 * wait 100us to satisfy "minimum reset assertion time from clock stable
	 * requirement of the PCI spec
	 */
	DELAY(100);
        /* PCI Reset deassert */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPOUTR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPOUTR, reg | (1U << GPIO_PCI_RESET));

	/*
	 * AHB->PCI address translation
	 *	PCI Memory Map allocation in 0x48000000 (64MB)
	 *	see. IXP425_PCI_MEM_HWBASE
	 */
	PCI_CSR_WRITE_4(sc, PCI_PCIMEMBASE, 0x48494a4b);

	/*
	 * PCI->AHB address translation
	 * 	begin at the physical memory start + OFFSET
	 */
#define	AHB_OFFSET	0x10000000UL
	PCI_CSR_WRITE_4(sc, PCI_AHBMEMBASE,
			 (AHB_OFFSET & 0xFF000000) +
			((AHB_OFFSET & 0xFF000000) >> 8) +
			((AHB_OFFSET & 0xFF000000) >> 16) +
			((AHB_OFFSET & 0xFF000000) >> 24) +
			  0x00010203);

	/* write Mapping registers PCI Configuration Registers */
	/* Base Address 0 - 3 */
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR0, AHB_OFFSET + 0x00000000);
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR1, AHB_OFFSET + 0x01000000);
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR2, AHB_OFFSET + 0x02000000);
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR3, AHB_OFFSET + 0x03000000);

	/* Base Address 4 */
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR4, 0xffffffff);

	/* Base Address 5 */
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR5, 0x00000000);

	/* assert some PCI errors */
	PCI_CSR_WRITE_4(sc, PCI_ISR, ISR_AHBE | ISR_PPE | ISR_PFE | ISR_PSE);

	/*
	 * Set up byte lane swapping between little-endian PCI
	 * and the big-endian AHB bus
	 */
	PCI_CSR_WRITE_4(sc, PCI_CSR, CSR_IC | CSR_ABE | CSR_PDS);

	/*
	 * Enable bus mastering and I/O,memory access
	 */
	ixp425_pci_conf_reg_write(sc, PCI_COMMAND_STATUS_REG,
		PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE |
		PCI_COMMAND_MASTER_ENABLE);
}

void
ixp425_md_pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int pin,
    int swiz, int *ilinep)
{

	if (bus == 0)
		*ilinep = ((swiz + (dev + pin - 1)) & 3);
	else
		panic("ixp425_md_pci_conf_interrupt: unsupported bus number");
}

#define	IXP425_MAX_DEV	4
#define	IXP425_MAX_LINE	4
static int
ixdp425_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	static int ixp425_pci_table[IXP425_MAX_DEV][IXP425_MAX_LINE] =
	{
		{PCI_INT_A, PCI_INT_B, PCI_INT_C, PCI_INT_D},
		{PCI_INT_B, PCI_INT_C, PCI_INT_D, PCI_INT_A},
		{PCI_INT_C, PCI_INT_D, PCI_INT_A, PCI_INT_B},
		{PCI_INT_D, PCI_INT_A, PCI_INT_B, PCI_INT_C},
	};

	int pin = pa->pa_intrpin;
	int dev = pa->pa_device;

#ifdef PCI_DEBUG
	void *v = pa->pa_pc;
	int line = pa->pa_intrline;
	pcitag_t intrtag = pa->pa_intrtag;

	printf("ixdp425_pci_intr_map: v=%p, tag=%08lx intrpin=%d line=%d dev=%d\n",
		v, intrtag, pin, line, dev);
#endif
	
	if (pin >= 1 && pin <= IXP425_MAX_LINE &&
	    dev >= 1 && dev <= IXP425_MAX_DEV) {
		*ihp = ixp425_pci_table[dev - 1][pin - 1];
		return (0);
	} else {
		printf("ixdp425_pci_intr_map: no mapping for %d/%d/%d\n",
			pa->pa_bus, pa->pa_device, pa->pa_function);
		return (1);
	}
}

static const char *
ixdp425_pci_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	snprintf(buf, len, "ixp425 irq %ld", ih);
	return buf;
}

static const struct evcnt *
ixdp425_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return (NULL);
}

static void *
ixdp425_pci_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*func)(void *), void *arg)
{
#ifdef PCI_DEBUG
	printf("ixdp425_pci_intr_establish(v=%p, irq=%d, ipl=%d, func=%p, arg=%p)\n",
		v, (int) ih, ipl, func, arg);
#endif

	return (ixp425_intr_establish(ih, ipl, func, arg));
}

static void
ixdp425_pci_intr_disestablish(void *v, void *cookie)
{
#ifdef PCI_DEBUG
	printf("ixdp425_pci_intr_disestablish(v=%p, cookie=%p)\n",
		v, cookie);
#endif

	ixp425_intr_disestablish(cookie);
}
