/*	$NetBSD: yeeloong_machdep.c,v 1.3.2.3 2014/08/20 00:02:58 tls Exp $	*/
/*	$OpenBSD: yeeloong_machdep.c,v 1.16 2011/04/15 20:40:06 deraadt Exp $	*/

/*
 * Copyright (c) 2009, 2010 Miodrag Vallat.
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

/*
 * Lemote {Fu,Lyn,Yee}loong specific code and configuration data.
 * (this file really ought to be named lemote_machdep.c by now)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: yeeloong_machdep.c,v 1.3.2.3 2014/08/20 00:02:58 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/types.h>

#include <evbmips/loongson/autoconf.h>
#include <mips/pmon/pmon.h>
#include <evbmips/loongson/loongson_intr.h>
#include <evbmips/loongson/loongson_bus_defs.h>
#include <evbmips/loongson/loongson_isa.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/i8259reg.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>

#include <evbmips/loongson/dev/kb3310var.h>
#include <evbmips/loongson/dev/glxreg.h>
#include <evbmips/loongson/dev/glxvar.h>

#include "com.h"
#include "isa.h"
#include "ykbec.h"

#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comvar.h>
#endif

#ifdef LOW_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

void	 lemote_device_register(device_t, void *);
void	 lemote_reset(void);

void	 fuloong_powerdown(void);
void	 fuloong_setup(void);

void	 yeeloong_powerdown(void);

void	 lemote_pci_attach_hook(device_t, device_t,
	    struct pcibus_attach_args *);
int	 lemote_intr_map(int, int, int, pci_intr_handle_t *);

void	 lemote_isa_attach_hook(device_t, device_t,
	    struct isabus_attach_args *);
void	*lemote_isa_intr_establish(void *, int, int, int,
	    int (*)(void *), void *);
void	 lemote_isa_intr_disestablish(void *, void *);
const struct evcnt * lemote_isa_intr_evcnt(void *, int);
const char * lemote_isa_intr_string(void *, int, char *, size_t);

uint	 lemote_get_isa_imr(void);
uint	 lemote_get_isa_isr(void);
void	 lemote_isa_intr(int, vaddr_t, uint32_t);

const struct bonito_config lemote_bonito = {
	.bc_adbase = 11,

	.bc_gpioIE = LOONGSON_INTRMASK_GPIO,
	.bc_intEdge = LOONGSON_INTRMASK_PCI_SYSERR |
	    LOONGSON_INTRMASK_PCI_PARERR,
	.bc_intSteer = 0,
	.bc_intPol = LOONGSON_INTRMASK_DRAM_PARERR |
	    LOONGSON_INTRMASK_PCI_SYSERR | LOONGSON_INTRMASK_PCI_PARERR |
	    LOONGSON_INTRMASK_INT0 | LOONGSON_INTRMASK_INT1,

	.bc_attach_hook = lemote_pci_attach_hook,
};

const struct legacy_io_range fuloong_legacy_ranges[] = {
	/* isa */
	{ IO_DMAPG + 4,	IO_DMAPG + 4 },
	/* mcclock */
	{ IO_RTC,	IO_RTC + 1 },
	/* pciide */
	{ 0x170,	0x170 + 7 },
	{ 0x1f0,	0x1f0 + 7 },
	{ 0x376,	0x376 },
	{ 0x3f6,	0x3f6 },
	/* com */
	{ IO_COM1,	IO_COM1 + 8 },		/* IR port */
	{ IO_COM2,	IO_COM2 + 8 },		/* serial port */

	{ 0 }
};

const struct legacy_io_range lynloong_legacy_ranges[] = {
	/* isa */
	{ IO_DMAPG + 4,	IO_DMAPG + 4 },
	/* mcclock */
	{ IO_RTC,	IO_RTC + 1 },
	/* pciide */
	{ 0x170,	0x170 + 7 },
	{ 0x1f0,	0x1f0 + 7 },
	{ 0x376,	0x376 },
	{ 0x3f6,	0x3f6 },
#if 0	/* no external connector */
	/* com */
	{ IO_COM2,	IO_COM2 + 8 },
#endif

	{ 0 }
};

const struct legacy_io_range yeeloong_legacy_ranges[] = {
	/* isa */
	{ IO_DMAPG + 4,	IO_DMAPG + 4 },
	/* pckbc */
	{ IO_KBD,	IO_KBD },
	{ IO_KBD + 4,	IO_KBD + 4 },
	/* mcclock */
	{ IO_RTC,	IO_RTC + 1 },
	/* pciide */
	{ 0x170,	0x170 + 7 },
	{ 0x1f0,	0x1f0 + 7 },
	{ 0x376,	0x376 },
	{ 0x3f6,	0x3f6 },
	/* kb3110b embedded controller */
	{ 0x381,	0x383 },

	{ 0 }
};

struct mips_isa_chipset lemote_isa_chipset = {
	.ic_v = NULL,

	.ic_attach_hook = lemote_isa_attach_hook,
	.ic_intr_establish = lemote_isa_intr_establish,
	.ic_intr_disestablish = lemote_isa_intr_disestablish,
	.ic_intr_evcnt = lemote_isa_intr_evcnt,
	.ic_intr_string = lemote_isa_intr_string,
};

const struct platform fuloong_platform = {
	.system_type = LOONGSON_FULOONG,
	.vendor = "Lemote",
	.product = "Fuloong",

	.bonito_config = &lemote_bonito,
	.isa_chipset = &lemote_isa_chipset,
	.legacy_io_ranges = fuloong_legacy_ranges,
	.bonito_mips_intr = MIPS_INT_MASK_4,
	.isa_mips_intr = MIPS_INT_MASK_0,
	.isa_intr = lemote_isa_intr,
	.p_pci_intr_map = lemote_intr_map,
	.irq_map =loongson2f_irqmap,

	.setup = fuloong_setup,
	.device_register = lemote_device_register,

	.powerdown = fuloong_powerdown,
	.reset = lemote_reset
};

const struct platform lynloong_platform = {
	.system_type = LOONGSON_LYNLOONG,
	.vendor = "Lemote",
	.product = "Lynloong",

	.bonito_config = &lemote_bonito,
	.isa_chipset = &lemote_isa_chipset,
	.legacy_io_ranges = lynloong_legacy_ranges,
	.bonito_mips_intr = MIPS_INT_MASK_4,
	.isa_mips_intr = MIPS_INT_MASK_0,
	.isa_intr = lemote_isa_intr,
	.p_pci_intr_map = lemote_intr_map,
	.irq_map =loongson2f_irqmap,

	.setup = fuloong_setup,
	.device_register = lemote_device_register,

	.powerdown = fuloong_powerdown,
	.reset = lemote_reset
};

const struct platform yeeloong_platform = {
	.system_type = LOONGSON_YEELOONG,
	.vendor = "Lemote",
	.product = "Yeeloong",

	.bonito_config = &lemote_bonito,
	.isa_chipset = &lemote_isa_chipset,
	.legacy_io_ranges = yeeloong_legacy_ranges,
	.bonito_mips_intr = MIPS_INT_MASK_4,
	.isa_mips_intr = MIPS_INT_MASK_0,
	.isa_intr = lemote_isa_intr,
	.p_pci_intr_map = lemote_intr_map,
	.irq_map =loongson2f_irqmap,

	.setup = NULL,
	.device_register = lemote_device_register,

	.powerdown = yeeloong_powerdown,
	.reset = lemote_reset,
#if NYKBEC > 0
	.suspend = ykbec_suspend,
	.resume = ykbec_resume
#endif
};

#if NISA > 0
static int stray_intr[BONITO_NISA];
#endif
/*
 * PCI model specific routines
 */

void
lemote_pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
	pci_chipset_tag_t pc = pba->pba_pc;
	pcitag_t tag;
	pcireg_t id;
	int dev, i;

	if (pba->pba_bus != 0)
		return;

	/*
	 * Check for an AMD CS5536 chip; if one is found, register
	 * the proper PCI configuration space hooks.
	 */

	for (dev = pci_bus_maxdevs(pc, 0); dev >= 0; dev--) {
		tag = pci_make_tag(pc, 0, dev, 0);
		id = pci_conf_read(pc, tag, PCI_ID_REG);
		DPRINTF(("lemote_pci_attach_hook id 0x%x\n", id));
		if (id == PCI_ID_CODE(PCI_VENDOR_AMD,
		    PCI_PRODUCT_AMD_CS5536_PCISB)) {
			glx_init(pc, tag, dev);
			break;
		}
	}

	wrmsr(GCSC_PIC_SHDW, 0);
	DPRINTF(("PMON setup picregs:"));
	for (i = 0; i < 12; i++) {
		if (i == 6)
			DPRINTF((" | "));
		DPRINTF((" 0x%x", (uint32_t)(rdmsr(GCSC_PIC_SHDW) & 0xff)));
	}
	DPRINTF(("\n"));
	DPRINTF(("intsel 0x%x 0x%x\n", REGVAL8(BONITO_PCIIO_BASE + 0x4d0),
	    REGVAL8(BONITO_PCIIO_BASE + 0x4d1)));

	/* setup legacy interrupt controller */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW1) = 0xff;
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_ICW1) =
	    ICW1_SELECT | ICW1_IC4;
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_ICW2) = ICW2_VECTOR(0);
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_ICW3) = ICW3_CASCADE(2);
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_ICW4) = ICW4_8086;
	delay(100);
	/* mask all interrupts */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW1) = 0xff;

	/* read ISR by default. */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW3) = OCW3_SELECT | OCW3_RR;
	(void)REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW3);

	/* reset; program device, four bytes */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW1) = 0xff;
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_ICW1) =
	    ICW1_SELECT | ICW1_IC4;
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_ICW2) = ICW2_VECTOR(8);
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_ICW3) = ICW3_SIC(2);
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_ICW4) = ICW4_8086;
	delay(100);
	/* leave interrupts masked */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW1) = 0xff;
	/* read ISR by default. */
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW3) = OCW3_SELECT | OCW3_RR;
	(void)REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW3);
}

int
lemote_intr_map(int dev, int fn, int pin, pci_intr_handle_t *ihp)
{
	switch (dev) {
	/* onboard devices, only pin A is wired */
	case 6:
	case 7:
	case 8:
	case 9:
		if (pin == PCI_INTERRUPT_PIN_A) {
			*ihp = BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIA +
			    (dev - 6));
			return (0);
		}
		break;
	/* PCI slot */
	case 10:
		*ihp = BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIA +
		    (pin - PCI_INTERRUPT_PIN_A));
		return (0);
	/* Geode chip */
	case 14:
		switch (fn) {
		case 1:	/* Flash */
			*ihp = BONITO_ISA_IRQ(6);
			return (0);
		case 3:	/* AC97 */
			*ihp = BONITO_ISA_IRQ(9);
			return (0);
		case 4:	/* OHCI */
		case 5:	/* EHCI */
			*ihp = BONITO_ISA_IRQ(11);
			return (0);
		}
		break;
	default:
		break;
	}
	return (1);
}

/*
 * ISA model specific routines
 */
#if NISA > 0
void
lemote_isa_attach_hook(device_t parent, device_t self,
    struct isabus_attach_args *iba)
{

	loongson_set_isa_imr(loongson_isaimr);
}

void *
lemote_isa_intr_establish(void *v, int irq, int type, int level,
    int (*handler)(void *), void *arg)
{
	void *ih;
	uint imr;

	ih =  evbmips_intr_establish(BONITO_ISA_IRQ(irq), handler, arg);
	if (ih == NULL)
		return (NULL);

	/* enable interrupt */
	imr = lemote_get_isa_imr();
	imr |= (1 << irq);
	DPRINTF(("lemote_isa_intr_establish: enable irq %d 0x%x\n", irq, imr));
	loongson_set_isa_imr(imr);
	return (ih);
}

void
lemote_isa_intr_disestablish(void *v, void *ih)
{

	evbmips_intr_disestablish(ih);
}

const struct evcnt *
lemote_isa_intr_evcnt(void *v, int irq)
{

        if (irq == 0 || irq >= BONITO_NISA || irq == 2)
		panic("lemote_isa_intr_evcnt: bogus isa irq 0x%x", irq);

	return (&bonito_intrhead[BONITO_ISA_IRQ(irq)].intr_count);
}

const char *
lemote_isa_intr_string(void *v, int irq, char *buf, size_t len)
{
	if (irq == 0 || irq >= BONITO_NISA || irq == 2)
		panic("lemote_isa_intr_string: bogus isa irq 0x%x", irq);

	return loongson_intr_string(&lemote_bonito, BONITO_ISA_IRQ(irq), buf,
	    len);
}
#endif
/*
 * Legacy (ISA) interrupt handling
 */

/*
 * Process legacy interrupts.
 *
 * XXX On 2F, ISA interrupts only occur on LOONGSON_INTR_INT0, but since
 * XXX the other LOONGSON_INTR_INT# are unmaskable, bad things will happen
 * XXX if they ever are triggered...
 */
void
lemote_isa_intr(int ipl, vaddr_t pc, uint32_t ipending)
{
#if NISA > 0
	struct evbmips_intrhand *ih;
	uint32_t isr, imr, mask;
	int bitno;
	int rc;

	imr = lemote_get_isa_imr();
	isr = lemote_get_isa_isr() & imr;
	if (isr == 0)
		return;

	/*
	 * Now process allowed interrupts.
	 */
	/* Service higher level interrupts first */
	for (bitno = BONITO_NISA - 1, mask = 1UL << bitno;
	     mask != 0;
	     bitno--, mask >>= 1) {
		if ((isr & mask) == 0)
			continue;

		loongson_isa_specific_eoi(bitno);

		rc = 0;
		LIST_FOREACH(ih,
		    &bonito_intrhead[BONITO_ISA_IRQ(bitno)].intrhand_head,
		    ih_q) {
			if ((*ih->ih_func)(ih->ih_arg) != 0) {
				rc = 1;
				bonito_intrhead[BONITO_ISA_IRQ(bitno)].intr_count.ev_count++;
			}
		}
		if (rc == 0) {
			if (stray_intr[bitno]++ & 0x10000) {
				printf("spurious isa interrupt %d\n", bitno);
				stray_intr[bitno] = 0;
			}
		}

		if ((isr ^= mask) == 0)
			break;
	}

	/*
	 * Reenable interrupts which have been serviced.
	 */
	loongson_set_isa_imr(imr);
#endif
}

uint
lemote_get_isa_imr(void)
{
	uint imr1, imr2;

	imr1 = 0xff & ~REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW1);
	imr1 &= ~(1 << 2);	/* hide cascade */
	imr2 = 0xff & ~REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW1);

	return ((imr2 << 8) | imr1);
}

uint
lemote_get_isa_isr(void)
{
	uint isr1, isr2;

	isr1 = REGVAL8(BONITO_PCIIO_BASE + IO_ICU1);
	isr1 &= ~(1 << 2);
	isr2 = REGVAL8(BONITO_PCIIO_BASE + IO_ICU2);

	return ((isr2 << 8) | isr1);
}

/*
 * Other model specific routines
 */

void
fuloong_powerdown(void)
{
	vaddr_t gpiobase;

	gpiobase = BONITO_PCIIO_BASE + (rdmsr(GCSC_DIVIL_LBAR_GPIO) & 0xff00);
	/* enable GPIO 13 */
	REGVAL(gpiobase + GCSC_GPIOL_OUT_EN) = GCSC_GPIO_ATOMIC_VALUE(13, 1);
	/* set GPIO13 value to zero */
	REGVAL(gpiobase + GCSC_GPIOL_OUT_VAL) = GCSC_GPIO_ATOMIC_VALUE(13, 0);
}

void
yeeloong_powerdown(void)
{

	REGVAL(BONITO_GPIODATA) &= ~0x00000001;
	REGVAL(BONITO_GPIOIE) &= ~0x00000001;
}

void
lemote_reset(void)
{

	wrmsr(GCSC_GLCP_SYS_RST, rdmsr(GCSC_GLCP_SYS_RST) | 1);
}

void
fuloong_setup(void)
{
#if NCOM > 0
	const char *envvar;
	int serial;

	envvar = pmon_getenv("nokbd");
	serial = envvar != NULL;
	envvar = pmon_getenv("novga");
	serial = serial && envvar != NULL;

	//serial = 1; /* XXXXXX */
	if (serial) {
                comconsiot = &bonito_iot;
                comconsaddr = 0x2f8;
                comconsrate = 115200; /* default PMON console speed */
	}
#endif
}

void
lemote_device_register(device_t dev, void *aux)
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
			    device_is_a(dev, "cd") == 0) &&
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
