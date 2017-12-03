/*	$OpenBSD: generic2e_machdep.c,v 1.2 2011/04/15 20:40:06 deraadt Exp $	*/

/*
 * Copyright (c) 2010 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Generic Loongson 2E code and configuration data.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: generic2e_machdep.c,v 1.2.6.3 2017/12/03 11:36:09 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/types.h>


#include <dev/ic/i8259reg.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <mips/cpuregs.h>
#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>

#include <evbmips/loongson/autoconf.h>
#include <mips/pmon/pmon.h>
#include <evbmips/loongson/loongson_intr.h>
#include <evbmips/loongson/loongson_isa.h>
#include <evbmips/loongson/loongson_bus_defs.h>

#include "com.h"
#include "isa.h"

#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comvar.h>
#endif

void	generic2e_device_register(device_t, void *);
void	generic2e_reset(void);

void	generic2e_setup(void);

void	generic2e_pci_attach_hook(device_t, device_t,
    struct pcibus_attach_args *);
int	generic2e_intr_map(int, int, int, pci_intr_handle_t *);

void	generic2e_isa_attach_hook(device_t, device_t,
	    struct isabus_attach_args *);
void	*generic2e_isa_intr_establish(void *, int, int, int,
	     int (*)(void *), void *);
void	generic2e_isa_intr_disestablish(void *, void *);
const struct evcnt * generic2e_isa_intr_evcnt(void *, int);
const char * generic2e_isa_intr_string(void *, int, char *, size_t);

void 	generic2e_isa_intr(int, vaddr_t, uint32_t);

void	via686sb_setup(pci_chipset_tag_t, int);

/* PnP IRQ assignment for VIA686 SuperIO components */
#define	VIA686_IRQ_PCIA		9
#define	VIA686_IRQ_PCIB		10
#define	VIA686_IRQ_PCIC		11
#define	VIA686_IRQ_PCID		13

static int generic2e_via686sb_dev = -1;

const struct bonito_config generic2e_bonito = {
	.bc_adbase = 11,

	.bc_gpioIE = 0xffffffff,
	.bc_intEdge = BONITO_INTRMASK_SYSTEMERR | BONITO_INTRMASK_MASTERERR |
	    BONITO_INTRMASK_RETRYERR | BONITO_INTRMASK_MBOX,
	.bc_intSteer = 0,
	.bc_intPol = 0,

	.bc_attach_hook = generic2e_pci_attach_hook,
};

const struct legacy_io_range generic2e_legacy_ranges[] = {
	/* no isa space access restrictions */
	{ 0,		BONITO_PCIIO_LEGACY },

	{ 0 }
};

struct mips_isa_chipset generic2e_isa_chipset = {
	.ic_v = NULL,

	.ic_attach_hook = generic2e_isa_attach_hook,
	.ic_intr_establish = generic2e_isa_intr_establish,
	.ic_intr_disestablish = generic2e_isa_intr_disestablish,
	.ic_intr_evcnt = generic2e_isa_intr_evcnt,
	.ic_intr_string = generic2e_isa_intr_string,
};

const struct platform generic2e_platform = {
	.system_type = LOONGSON_2E,
	.vendor = "Generic",
	.product = "Loongson2E",

	.bonito_config = &generic2e_bonito,
	.isa_chipset = &generic2e_isa_chipset,
	.legacy_io_ranges = generic2e_legacy_ranges,
	.bonito_mips_intr = MIPS_INT_MASK_0,
	.isa_mips_intr = MIPS_INT_MASK_3,
	.isa_intr = generic2e_isa_intr,
	.p_pci_intr_map = generic2e_intr_map,
	.irq_map = loongson2e_irqmap,

	.setup = generic2e_setup,
	.device_register = generic2e_device_register,

	.powerdown = NULL,
	.reset = generic2e_reset
};

/*
 * PCI model specific routines
 */

void
generic2e_pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
	pci_chipset_tag_t pc = pba->pba_pc;
	pcireg_t id;
	pcitag_t tag;
	int dev;

	if (pba->pba_bus != 0)
		return;

	/*
	 * Check for a VIA 686 southbridge; if one is found, remember
	 * its location, needed by generic2e_intr_map().
	 */

	for (dev = pci_bus_maxdevs(pc, 0); dev >= 0; dev--) {
		tag = pci_make_tag(pc, 0, dev, 0);
		id = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id == PCI_ID_CODE(PCI_VENDOR_VIATECH,
		    PCI_PRODUCT_VIATECH_VT82C686A_ISA)) {
			generic2e_via686sb_dev = dev;
			break;
		}
	}

	if (generic2e_via686sb_dev != 0)
		via686sb_setup(pc, generic2e_via686sb_dev);
}

int
generic2e_intr_map(int dev, int fn, int pin, pci_intr_handle_t *ihp)
{
	if (dev == generic2e_via686sb_dev) {
		switch (fn) {
		case 1:	/* PCIIDE */
			/* will use compat interrupt */
			break;
		case 2:	/* USB */
			*ihp = BONITO_ISA_IRQ(VIA686_IRQ_PCIB);
			return 0;
		case 3:	/* USB */
			*ihp = BONITO_ISA_IRQ(VIA686_IRQ_PCIC);
			return 0;
		case 4:	/* power management, SMBus */
			break;
		case 5:	/* Audio */
			*ihp = BONITO_ISA_IRQ(VIA686_IRQ_PCIA);
			return 0;
		case 6:	/* Modem */
			break;
		default:
			break;
		}
	} else {
		*ihp = BONITO_DIRECT_IRQ(BONITO_INTR_GPIN +
		    pin - PCI_INTERRUPT_PIN_A);
		return 0;
	}

	return 1;
}

/*
 * ISA model specific routines
 */

void
generic2e_isa_attach_hook(device_t parent, device_t self,
    struct isabus_attach_args *iba)
{
	loongson_set_isa_imr(loongson_isaimr);
}

void *
generic2e_isa_intr_establish(void *v, int irq, int type, int level,
    int (*handler)(void *), void *arg)
{
	void *ih;
	uint imr;

	ih = evbmips_intr_establish(BONITO_ISA_IRQ(irq), handler, arg);
	if (ih == NULL)
		return NULL;
	/* enable interrupt */
	imr = loongson_isaimr;
	imr |= (1 << irq);
	loongson_set_isa_imr(imr); 
	return ih;
}

void
generic2e_isa_intr_disestablish(void *v, void *ih)
{
	evbmips_intr_disestablish(ih);
}

const struct evcnt *
generic2e_isa_intr_evcnt(void *v, int irq)
{

        if (irq == 0 || irq >= BONITO_NISA || irq == 2)
		panic("generic2e_isa_intr_evcnt: bogus isa irq 0x%x", irq);

	return (&bonito_intrhead[BONITO_ISA_IRQ(irq)].intr_count);
}

const char *
generic2e_isa_intr_string(void *v, int irq, char *buf, size_t len)
{
	if (irq == 0 || irq >= BONITO_NISA || irq == 2)
		panic("generic2e_isa_intr_string: bogus isa irq 0x%x", irq);

	return loongson_intr_string(&generic2e_bonito, irq, buf, len);
}

void
generic2e_isa_intr(int ipl, vaddr_t pc, uint32_t ipending)
{
#if NISA > 0
	struct evbmips_intrhand *ih;
	uint64_t isr, mask = 0;
	int rc, irq, ret;
	uint8_t ocw1, ocw2;

	for (;;) {
		REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW3) = 
		    OCW3_SELECT | OCW3_POLL;
		ocw1 = REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW3);
		if ((ocw1 & OCW3_POLL_PENDING) == 0)
			break;

		irq = OCW3_POLL_IRQ(ocw1);

		if (irq == 2) /* cascade */ {
			REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW3) = 
			    OCW3_SELECT | OCW3_POLL;
			ocw2 = REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW3);
			if (ocw2 & OCW3_POLL_PENDING)
				irq = OCW3_POLL_IRQ(ocw2);
			else
				irq = 2;
		} else
			ocw2 = 0;

		/*
		 * Mask the interrupt before servicing it.
		 */
		isr = 1UL << irq;
		loongson_set_isa_imr(loongson_isaimr & ~isr);

		mask |= isr;

		rc = 0;
		LIST_FOREACH(ih,
		    &bonito_intrhead[BONITO_ISA_IRQ(irq)].intrhand_head,
		    ih_q) {
			ret = (*ih->ih_func)(ih->ih_arg);
			if (ret) {
				rc = 1;
				bonito_intrhead[BONITO_ISA_IRQ(irq)].intr_count.ev_count++;
			}

			if (ret == 1)
				break;
		}

		/* Send a specific EOI to the 8259. */
		loongson_isa_specific_eoi(irq);

		if (rc == 0) {
			printf("spurious isa interrupt %d\n", irq);
		}
	}

	/*
	 * Reenable interrupts which have been serviced.
	 */
	if (mask != 0)
		loongson_set_isa_imr(loongson_isaimr | mask);

#endif
}

/*
 * Other model specific routines
 */

void
generic2e_reset(void)
{
	REGVAL(BONITO_BONGENCFG) &= ~BONITO_BONGENCFG_CPUSELFRESET;
	REGVAL(BONITO_BONGENCFG) |= BONITO_BONGENCFG_CPUSELFRESET;
	delay(1000000);
}

void
generic2e_setup(void)
{
#if NCOM > 0
	const char *envvar;
	int serial;

	envvar = pmon_getenv("nokbd");
	serial = envvar != NULL;
	envvar = pmon_getenv("novga");
	serial = serial && envvar != NULL;

	if (serial) {
                comconsiot = &bonito_iot;
                comconsaddr = 0x3f8;
                comconsrate = 115200; /* default PMON console speed */
	}
#endif
}

void
generic2e_device_register(device_t dev, void *aux)
{
	const char *name = device_xname(dev);

	if (device_class(dev) != bootdev_class)
		return;

	/* 
	 * The device numbering must match. There's no way
	 * pmon tells us more info. Depending on the usb slot
	 * and hubs used you may be lucky. Also, assume umass/sd for usb
	 * attached devices.
	 */
	switch (bootdev_class) {
	case DV_DISK:
		if (device_is_a(dev, "wd") && strcmp(name, bootdev) == 0) {
			if (booted_device == NULL)
				booted_device = dev;
		} else {
			/* XXX this really only works safely for usb0... */
		    	if ((device_is_a(dev, "sd") ||
			    device_is_a(dev, "cd")) &&
			    strncmp(bootdev, "usb", 3) == 0 &&
			    strcmp(name + 2, bootdev + 3) == 0) {
				if (booted_device == NULL)
					booted_device = dev;
			}
		}
		break;
	case DV_IFNET:
		/*
		 * This relies on the onboard Ethernet interface being
		 * attached before any other (usb) interface.
		 */
		if (booted_device == NULL)
			booted_device = dev;
		break;
	default:
		break;
	}
}

/*
 * Initialize a VIA686 south bridge.
 *
 * PMON apparently does not perform enough initialization; one may argue this
 * could be done with a specific pcib(4) driver, but then no other system
 * will hopefully need this, so keep it local to the 2E setup code.
 */

#define	VIA686_ISA_ROM_CONTROL		0x40
#define	VIA686_ROM_WRITE_ENABLE			0x00000001
#define	VIA686_NO_ROM_WAIT_STATE		0x00000002
#define	VIA686_EXTEND_ALE			0x00000004
#define	VIA686_IO_RECOVERY_TIME			0x00000008
#define	VIA686_CHIPSET_EXTRA_WAIT_STATES	0x00000010
#define	VIA686_ISA_EXTRA_WAIT_STATES		0x00000020
#define	VIA686_ISA_EXTENDED_BUS_READY		0x00000040
#define	VIA686_ISA_EXTRA_COMMAND_DELAY		0x00000080
#define	VIA686_ISA_REFRESH			0x00000100
#define	VIA686_DOUBLE_DMA_CLOCK			0x00000800
#define	VIA686_PORT_92_FAST_RESET		0x00002000
#define	VIA686_IO_MEDIUM_RECOVERY_TIME		0x00004000
#define	VIA686_KBC_DMA_MISC12		0x44
#define	VIA686_ISA_MASTER_TO_LINE_BUFFER	0x00008000
#define	VIA686_POSTED_MEMORY_WRITE_ENABLE	0x00010000
#define	VIA686_PCI_BURST_INTERRUPTABLE		0x00020000
#define	VIA686_FLUSH_LINE_BUFFER_ON_INTR	0x00200000
#define	VIA686_GATE_INTR			0x00400000
#define	VIA686_PCI_MASTER_WRITE_WAIT_STATE	0x00800000
#define	VIA686_PCI_RESET			0x01000000
#define	VIA686_PCI_READ_DELAY_TRANSACTION_TMO	0x02000000
#define	VIA686_PCI_WRITE_DELAY_TRANSACTION_TMO	0x04000000
#define	VIA686_ICR_SHADOW_ENABLE		0x10000000
#define	VIA686_EISA_PORT_4D0_4D1_ENABLE		0x20000000
#define	VIA686_PCI_DELAY_TRANSACTION_ENABLE	0x40000000
#define	VIA686_CPU_RESET_SOURCE_INIT		0x80000000
#define	VIA686_MISC3_IDE_INTR		0x48
#define	VIA686_IDE_PRIMARY_CHAN_MASK		0x00030000
#define	VIA686_IDE_PRIMARY_CHAN_SHIFT			16
#define	VIA686_IDE_SECONDARY_CHAN_MASK		0x000c0000
#define	VIA686_IDE_SECONDARY_CHAN_SHIFT			18
#define	VIA686_IDE_IRQ14	00
#define	VIA686_IDE_IRQ15	01
#define	VIA686_IDE_IRQ10	02
#define	VIA686_IDE_IRQ11	03
#define	VIA686_IDE_PGNT				0x00800000
#define	VIA686_PNP_DMA_IRQ		0x50
#define	VIA686_DMA_FDC_MASK			0x00000003
#define	VIA686_DMA_FDC_SHIFT				0
#define	VIA686_DMA_LPT_MASK			0x0000000c
#define	VIA686_DMA_LPT_SHIFT				2
#define	VIA686_IRQ_FDC_MASK			0x00000f00
#define	VIA686_IRQ_FDC_SHIFT				8
#define	VIA686_IRQ_LPT_MASK			0x0000f000
#define	VIA686_IRQ_LPT_SHIFT				12
#define	VIA686_IRQ_COM0_MASK			0x000f0000
#define	VIA686_IRQ_COM0_SHIFT				16
#define	VIA686_IRQ_COM1_MASK			0x00f00000
#define	VIA686_IRQ_COM1_SHIFT				20
#define	VIA686_PCI_LEVEL_PNP_IRQ2	0x54
#define	VIA686_PCI_IRQD_EDGE			0x00000001
#define	VIA686_PCI_IRQC_EDGE			0x00000002
#define	VIA686_PCI_IRQB_EDGE			0x00000004
#define	VIA686_PCI_IRQA_EDGE			0x00000008
#define	VIA686_IRQ_PCIA_MASK			0x0000f000
#define	VIA686_IRQ_PCIA_SHIFT				12
#define	VIA686_IRQ_PCIB_MASK			0x000f0000
#define	VIA686_IRQ_PCIB_SHIFT				16
#define	VIA686_IRQ_PCIC_MASK			0x00f00000
#define	VIA686_IRQ_PCIC_SHIFT				20
#define	VIA686_IRQ_PCID_MASK			0xf0000000
#define	VIA686_IRQ_PCID_SHIFT				28

void
via686sb_setup(pci_chipset_tag_t pc, int dev)
{
	pcitag_t tag;
	pcireg_t reg;
	uint elcr;

	tag = pci_make_tag(pc, 0, dev, 0);

	/*
	 * Generic ISA bus initialization.
	 */

	reg = pci_conf_read(pc, tag, VIA686_ISA_ROM_CONTROL);
	reg |= VIA686_IO_RECOVERY_TIME | VIA686_ISA_REFRESH;
	pci_conf_write(pc, tag, VIA686_ISA_ROM_CONTROL, reg);

	reg = pci_conf_read(pc, tag, VIA686_KBC_DMA_MISC12);
	reg |= VIA686_CPU_RESET_SOURCE_INIT |
	    VIA686_PCI_DELAY_TRANSACTION_ENABLE |
	    VIA686_EISA_PORT_4D0_4D1_ENABLE |
	    VIA686_PCI_WRITE_DELAY_TRANSACTION_TMO |
	    VIA686_PCI_READ_DELAY_TRANSACTION_TMO |
	    VIA686_PCI_MASTER_WRITE_WAIT_STATE | VIA686_GATE_INTR |
	    VIA686_FLUSH_LINE_BUFFER_ON_INTR;
	reg &= ~VIA686_ISA_MASTER_TO_LINE_BUFFER;
	pci_conf_write(pc, tag, VIA686_KBC_DMA_MISC12, reg);

	/*
	 * SuperIO devices interrupt and DMA setup.
	 */

	reg = pci_conf_read(pc, tag, VIA686_MISC3_IDE_INTR);
	reg &= ~(VIA686_IDE_PRIMARY_CHAN_MASK | VIA686_IDE_SECONDARY_CHAN_MASK);
	reg |= (VIA686_IDE_IRQ14 << VIA686_IDE_PRIMARY_CHAN_SHIFT);
	reg |= (VIA686_IDE_IRQ15 << VIA686_IDE_SECONDARY_CHAN_SHIFT);
	reg |= VIA686_IDE_PGNT;
	pci_conf_write(pc, tag, VIA686_MISC3_IDE_INTR, reg);

	reg = pci_conf_read(pc, tag, VIA686_PNP_DMA_IRQ);
	reg &= ~(VIA686_DMA_FDC_MASK | VIA686_DMA_LPT_MASK);
	reg |= (2 << VIA686_DMA_FDC_SHIFT) | (3 << VIA686_DMA_LPT_SHIFT);
	reg &= ~(VIA686_IRQ_FDC_MASK | VIA686_IRQ_LPT_MASK);
	reg |= (6 << VIA686_IRQ_FDC_SHIFT) | (7 << VIA686_IRQ_LPT_SHIFT);
	reg &= ~(VIA686_IRQ_COM0_MASK | VIA686_IRQ_COM1_MASK);
	reg |= (4 << VIA686_IRQ_COM0_SHIFT) | (3 << VIA686_IRQ_COM1_SHIFT);

	reg = pci_conf_read(pc, tag, VIA686_PCI_LEVEL_PNP_IRQ2);
	reg &= ~(VIA686_PCI_IRQA_EDGE | VIA686_PCI_IRQB_EDGE |
	    VIA686_PCI_IRQB_EDGE | VIA686_PCI_IRQD_EDGE);
	reg &= ~(VIA686_IRQ_PCIA_MASK | VIA686_IRQ_PCIB_MASK |
	    VIA686_IRQ_PCIC_MASK | VIA686_IRQ_PCID_MASK);
	reg |= (VIA686_IRQ_PCIA << VIA686_IRQ_PCIA_SHIFT) |
	    (VIA686_IRQ_PCIB << VIA686_IRQ_PCIB_SHIFT) |
	    (VIA686_IRQ_PCIC << VIA686_IRQ_PCIC_SHIFT) |
	    (VIA686_IRQ_PCID << VIA686_IRQ_PCID_SHIFT);

	/*
	 * Interrupt controller setup.
	 */

	/* reset; program device, four bytes */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_ICW1) =
	    ICW1_SELECT | ICW1_IC4;
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_ICW2) = ICW2_VECTOR(0);
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_ICW3) = ICW3_CASCADE(2);
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_ICW4) = ICW4_8086;
	/* leave interrupts masked */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW1) = 0xff;
	/* special mask mode (if available) */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW3) =
	    OCW3_SELECT | OCW3_SSMM | OCW3_SMM;
	/* read IRR by default. */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW3) = OCW3_SELECT | OCW3_RR;

	/* reset; program device, four bytes */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_ICW1) =
	    ICW1_SELECT | ICW1_IC4;
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_ICW2) = ICW2_VECTOR(8);
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_ICW3) = ICW3_SIC(2);
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_ICW4) = ICW4_8086;
	/* leave interrupts masked */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW1) = 0xff;
	/* special mask mode (if available) */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW3) =
	    OCW3_SELECT | OCW3_SSMM | OCW3_SMM;
	/* read IRR by default. */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW3) = OCW3_SELECT | OCW3_RR;

	/* setup ELCR: PCI interrupts are level-triggered. */
	elcr = (1 << VIA686_IRQ_PCIA) | (1 << VIA686_IRQ_PCIB) |
	    (1 << VIA686_IRQ_PCIC) | (1 << VIA686_IRQ_PCID);
	REGVAL8(BONITO_PCIIO_BASE + 0x4d0) = (elcr >> 0) & 0xff;
	REGVAL8(BONITO_PCIIO_BASE + 0x4d1) = (elcr >> 8) & 0xff;

	__asm__ __volatile__ ("sync" ::: "memory");

	/*
	 * Update interrupt information for secondary functions.
	 * Although this information is not used by pci_intr_establish()
	 * because of generic2e_intr_map() behaviour, it seems to be
	 * required to complete proper interrupt routing.
	 */

	tag = pci_make_tag(pc, 0, dev, 2);
	reg = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
	reg &= ~(PCI_INTERRUPT_LINE_MASK << PCI_INTERRUPT_LINE_SHIFT);
	reg |= VIA686_IRQ_PCIB << PCI_INTERRUPT_LINE_SHIFT;
	pci_conf_write(pc, tag, PCI_INTERRUPT_REG, reg);

	tag = pci_make_tag(pc, 0, dev, 3);
	reg = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
	reg &= ~(PCI_INTERRUPT_LINE_MASK << PCI_INTERRUPT_LINE_SHIFT);
	reg |= VIA686_IRQ_PCIC << PCI_INTERRUPT_LINE_SHIFT;
	pci_conf_write(pc, tag, PCI_INTERRUPT_REG, reg);

	tag = pci_make_tag(pc, 0, dev, 5);
	reg = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
	reg &= ~(PCI_INTERRUPT_LINE_MASK << PCI_INTERRUPT_LINE_SHIFT);
	reg |= VIA686_IRQ_PCIA << PCI_INTERRUPT_LINE_SHIFT;
	pci_conf_write(pc, tag, PCI_INTERRUPT_REG, reg);
}
