/*      $NetBSD: nslu2_pci.c,v 1.1.10.2 2006/04/22 11:37:24 simonb Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: nslu2_pci.c,v 1.1.10.2 2006/04/22 11:37:24 simonb Exp $");

/*
 * Linksys NSLU2 PCI support.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <dev/pci/pcivar.h>

#include <evbarm/nslu2/nslu2reg.h>

static int
nslu2_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{

	KASSERT(pa->pa_bus == 0 && pa->pa_device == 1);

	switch (pa->pa_function) {
	case 0:
		*ihp = PCI_INT_A;
		break;

	case 1:
		*ihp = PCI_INT_B;
		break;

	case 2:
		*ihp = PCI_INT_C;
		break;

	default:
		return (1);
	}

	return (0);
}

static const char *
nslu2_pci_intr_string(void *v, pci_intr_handle_t ih)
{

	switch (ih) {
	case PCI_INT_A:
		return ("INTA");

	case PCI_INT_B:
		return ("INTB");

	case PCI_INT_C:
		return ("INTC");
	}

	return (NULL);
}

static const struct evcnt *
nslu2_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	return (NULL);
}

static void *
nslu2_pci_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*func)(void *), void *arg)
{

	return (ixp425_intr_establish(ih, ipl, func, arg));
}

static void
nslu2_pci_intr_disestablish(void *v, void *cookie)
{

	ixp425_intr_disestablish(cookie);
}

void
ixp425_md_pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int pin,
    int swiz, int *ilinep)
{

	KASSERT(bus == 0 && dev == 1);

	*ilinep = ((swiz + pin - 1) & 3);
}

void
ixp425_md_pci_init(struct ixp425_softc *sc)
{
	pci_chipset_tag_t pc = &sc->ia_pci_chipset;
	u_int32_t reg;

	pc->pc_intr_v = sc;
	pc->pc_intr_map = nslu2_pci_intr_map;
	pc->pc_intr_string = nslu2_pci_intr_string;
	pc->pc_intr_evcnt = nslu2_pci_intr_evcnt;
	pc->pc_intr_establish = nslu2_pci_intr_establish;
	pc->pc_intr_disestablish = nslu2_pci_intr_disestablish;

	/* PCI Reset Assert */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPOUTR);
	reg &= ~(1u << GPIO_PCI_RESET);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPOUTR, reg);

	/* PCI Clock Disable */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPCLKR);
	reg &= ~GPCLKR_MUX14;
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPCLKR, reg);

	/*
	 * Set GPIO Direction
	 *	Output: PCI_CLK, PCI_RESET
	 *	Input:  PCI_INTA, PCI_INTB, PCI_INTC
	 */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPOER);
	reg &= ~((1u << GPIO_PCI_CLK) | (1u << GPIO_PCI_RESET));
	reg |= (1u << GPIO_PCI_INTA) | (1u << GPIO_PCI_INTB) |
	    (1u << GPIO_PCI_INTC);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPOER, reg);

	/*
	 * Set GPIO interrupt type
	 *      PCI_INT_A, PCI_INTB, PCI_INT_C: Active Low
	 */
	reg = GPIO_CONF_READ_4(sc, GPIO_TYPE_REG(GPIO_PCI_INTA));
	reg &= ~GPIO_TYPE(GPIO_PCI_INTA, GPIO_TYPE_MASK);
	reg |= GPIO_TYPE(GPIO_PCI_INTA, GPIO_TYPE_ACT_LOW);
	GPIO_CONF_WRITE_4(sc, GPIO_TYPE_REG(GPIO_PCI_INTA), reg);

	reg = GPIO_CONF_READ_4(sc, GPIO_TYPE_REG(GPIO_PCI_INTB));
	reg &= ~GPIO_TYPE(GPIO_PCI_INTB, GPIO_TYPE_MASK);
	reg |= GPIO_TYPE(GPIO_PCI_INTB, GPIO_TYPE_ACT_LOW);
	GPIO_CONF_WRITE_4(sc, GPIO_TYPE_REG(GPIO_PCI_INTB), reg);

	reg = GPIO_CONF_READ_4(sc, GPIO_TYPE_REG(GPIO_PCI_INTC));
	reg &= ~GPIO_TYPE(GPIO_PCI_INTC, GPIO_TYPE_MASK);
	reg |= GPIO_TYPE(GPIO_PCI_INTC, GPIO_TYPE_ACT_LOW);
	GPIO_CONF_WRITE_4(sc, GPIO_TYPE_REG(GPIO_PCI_INTC), reg);

	/* Clear ISR */
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPISR, (1u << GPIO_PCI_INTA) |
	    (1u << GPIO_PCI_INTB) | (1u << GPIO_PCI_INTC));

	/* Wait 1ms to satisfy "minimum reset assertion time" of the PCI spec */
	DELAY(1000);
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPCLKR);
	reg |= (0xf << GPCLKR_CLK0DC_SHIFT) | (0xf << GPCLKR_CLK0TC_SHIFT);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPCLKR, reg);

	/* PCI Clock Enable */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPCLKR);
	reg |= GPCLKR_MUX14;
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPCLKR, reg);

	/*
	 * Wait 100us to satisfy "minimum reset assertion time from clock stable
	 * requirement of the PCI spec
	 */
	DELAY(100);
        /* PCI Reset deassert */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPOUTR);
	reg |= 1u << GPIO_PCI_RESET;
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPOUTR, reg);

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
	reg  = (AHB_OFFSET + 0x00000000) >> 0;
	reg |= (AHB_OFFSET + 0x01000000) >> 8;
	reg |= (AHB_OFFSET + 0x02000000) >> 16;
	reg |= (AHB_OFFSET + 0x03000000) >> 24;
	PCI_CSR_WRITE_4(sc, PCI_AHBMEMBASE, reg);

	/* Write Mapping registers PCI Configuration Registers */
	/* Base Address 0 - 3 */
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR0, AHB_OFFSET + 0x00000000);
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR1, AHB_OFFSET + 0x01000000);
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR2, AHB_OFFSET + 0x02000000);
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR3, AHB_OFFSET + 0x03000000);

	/* Base Address 4 */
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR4, 0xffffffff);

	/* Base Address 5 */
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR5, 0x00000000);

	/* Assert some PCI errors */
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

	/*
	 * Wait some more to ensure PCI devices have stabilised.
	 */
	DELAY(50000);
}
