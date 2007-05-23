/*	$NetBSD: pci_machdep.c,v 1.12.38.3 2007/05/23 01:45:10 nisimura Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-specific functions for PCI autoconfiguration.
 *
 * On PCs, there are two methods of generating PCI configuration cycles.
 * We try to detect the appropriate mechanism for this machine and set
 * up a few function pointers to access the correct method directly.
 *
 * The configuration method can be hard-coded in the config file by
 * using `options PCI_CONF_MODE=N', where `N' is the configuration mode
 * as defined section 3.6.4.1, `Generating Configuration Cycles'.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.12.38.3 2007/05/23 01:45:10 nisimura Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <uvm/uvm.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pcidevs.h>

struct powerpc_bus_dma_tag pci_bus_dma_tag = {
	0,			/* _bounce_thresh */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,			/* _dmamap_sync */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

static int brdtype;
#define BRD_SANDPOINTX2		2
#define BRD_SANDPOINTX3		3
#define BRD_ENCOREPP1		10
#define BRD_KUROBOX		100
#define BRD_QNAPTS101		101
#define BRD_SYNOLOGY		102
#define BRD_UNKNOWN		-1

#define	PCI_CONFIG_ENABLE	0x80000000UL

void
pci_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
	pcitag_t tag;
	pcireg_t dev11, dev22, dev15;

	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 11, 0);
	dev11 = pci_conf_read(pba->pba_pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(dev11) == PCI_CLASS_BRIDGE) {
		/* WinBond/Symphony Lab 83C553 at dev 11 */
		/*
		 * XXX distinguish SP3 from SP2 by fiddling ISA GPIO #7/6.
		 * XXX SP3 #7 output values loopback to #6 input.
		 */
		brdtype = BRD_SANDPOINTX3;
		return;
	}
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 22, 0);
	dev22 = pci_conf_read(pba->pba_pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(dev22) == PCI_CLASS_BRIDGE) {
		/* VIA 82C686B at dev 22 */
		brdtype = BRD_ENCOREPP1;
		return;
	}
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 11, 0);
	dev11 = pci_conf_read(pba->pba_pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(dev11) == PCI_CLASS_NETWORK) {
		/* tlp (ADMtek AN985) or re (RealTek 8169S) at dev 11 */
		brdtype = BRD_KUROBOX;
		return;
	}
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 15, 0);
	dev15 = pci_conf_read(pba->pba_pc, tag, PCI_ID_REG);
	if (PCI_VENDOR(dev15) == PCI_VENDOR_MARVELL) {
		/* Marvell GbE at dev 15 */
		brdtype = BRD_SYNOLOGY;
		return;
	}
	if (PCI_VENDOR(dev15) == PCI_VENDOR_INTEL) {
		/* Intel GbE at dev 15 */
		brdtype = BRD_QNAPTS101;
		return;
	}
	brdtype = BRD_UNKNOWN;
}

int
pci_bus_maxdevs(pc, busno)
	pci_chipset_tag_t pc;
	int busno;
{

	/*
	 * Bus number is irrelevant.  Configuration Mechanism 1 is in
	 * use, can have devices 0-32 (i.e. the `normal' range).
	 */
	return (32);
}

pcitag_t
pci_make_tag(pc, bus, device, function)
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	tag = PCI_CONFIG_ENABLE |
		    (bus << 16) | (device << 11) | (function << 8);
	return tag;
}

void
pci_decompose_tag(pc, tag, bp, dp, fp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int *bp, *dp, *fp;
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
	return;
}

/*
 * The Kahlua documentation says that "reg" should be left-shifted by two
 * and be in bits 2-7.  Apparently not.  It doesn't work that way, and the
 * DINK32 ROM doesn't do it that way (I peeked at 0xfec00000 after running
 * the DINK32 "pcf" command).
 */
#define SP_PCI(tag, reg) ((tag) | (reg))

pcireg_t
pci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	pcireg_t data;

	out32rb(SANDPOINT_PCI_CONFIG_ADDR, SP_PCI(tag,reg));
	data = in32rb(SANDPOINT_PCI_CONFIG_DATA);
	out32rb(SANDPOINT_PCI_CONFIG_ADDR, 0);
	return data;
}

void
pci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	out32rb(SANDPOINT_PCI_CONFIG_ADDR, SP_PCI(tag, reg));
	out32rb(SANDPOINT_PCI_CONFIG_DATA, data);
	out32rb(SANDPOINT_PCI_CONFIG_ADDR, 0);
}

int
pci_intr_map(pa, ihp)
	struct pci_attach_args *pa;
	pci_intr_handle_t *ihp;
{
	int	pin = pa->pa_intrpin;
	int	line = pa->pa_intrline;

	if (pin == 0) {
		/* No IRQ used. */
		goto bad;
	}

	if (pin > 4) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

	/*
	 * Section 6.2.4, `Miscellaneous Functions', says that 255 means
	 * `unknown' or `no connection' on a PC.  We assume that a device with
	 * `no connection' either doesn't have an interrupt (in which case the
	 * pin number should be 0, and would have been noticed above), or
	 * wasn't configured by the BIOS (in which case we punt, since there's
	 * no real way we can know how the interrupt lines are mapped in the
	 * hardware).
	 *
	 * XXX
	 * Since IRQ 0 is only used by the clock, and we can't actually be sure
	 * that the BIOS did its job, we also recognize that as meaning that
	 * the BIOS has not configured the device.
	 */
	if (line == 255) {
		printf("pci_intr_map: no mapping for pin %c\n", '@' + pin);
		goto bad;
	}

printf("line %d, pin %c", line, pin + '@');
	switch (brdtype) {
	/* Sandpoint has 4 PCI slots in a weird order.
	 * From next to MPMC mezzanine card toward the board edge,
	 * 	64bit slot PCI AD14
	 * 	64bit slot PCI AD13
	 * 	32bit slot PCI AD16
	 * 	32bit slot PCI AD15
	 * Don't believe identifying labels printed on PCB and
	 * documents confusing as well since Moto names the slots
	 * as number 1 origin.
	 *
	 * Sandpoint X3 "SP3" brd uses EPIC serial mode IRQ.  WinBond
	 * SB i8259 PIC interrupt is wired to EPIC IRQ0 while AD13-16
	 * come through IRQ2-5.
	 *
	 * Sandpoint X2 brd uses EPIC direct mode IRQ.  Interrupts
	 * from AD13-AD16 are wired with EPIC IRQ0-3.  WinBond SB
	 * i8259 shares EPIC IRQ1 line with the PCI slot next to
	 * MPMC mezzanine card.  WinBond IDE shares EPIC IRQ2 line.
	 */
	case BRD_SANDPOINTX3:
		if (line == 11
		    && pa->pa_function == 1 && pa->pa_bus == 0) {
			/* map pin A-D to EPIC IRQ6-9 */
			*ihp = 6 + (pin - 1);
			break;
		}
		if (line < 13 || line > 16) {
			printf("pci_intr_map: bad interrupt line %d,%c\n",
				line, pin + '@');
			goto bad;
		}
		/* map line 13-16 to EPIC IRQ2-5 */
		*ihp = line - 11;
		break;
	case BRD_SANDPOINTX2:
		if (line == 11
		    && pa->pa_function == 1 && pa->pa_bus == 0) {
			/* 83C553 PCI IDE comes thru EPIC IRQ2 */
			*ihp = 2;
			break;
		}
		if (line < 13 || line > 16) {
			printf("pci_intr_map: bad interrupt line %d,%c\n",
				line, pin + '@');
			goto bad;
		}
		/* map line 13-16 to EPIC IRQ0-3 */
		line -= 13; pin -= 1;
		*ihp = (line + (4 - pin)) & 3;
		break;
	case BRD_ENCOREPP1:
	/*
	 * Ampro EnCorePP1 brd uses EPIC direct mode IRQ. Via 686B SB
	 * i8259 interrupt goes through EPC IRQ0.  PCI pin A-D are
	 * tied with EPIC IRQ1-4.
	 * AD22 pin A,B,C,D -> EPIC IRQ 1,2,3,4.
	 * AD23 pin A,B,C,D -> EPIC IRQ 2,3,4,1.
	 * AD24 pin A,B,C,D -> EPIC IRQ 3,4,1,2.
	 * AD25 pin A,B,C,D -> EPIC IRQ 4,1,2,3.
	 */
		line -= 22; pin -= 1;
		*ihp = 1 + ((pin + line) & 3);
		break;
	case BRD_KUROBOX:
		/* map line 11,12,13,14 to EPIC IRQ0,1,4,3 */
		*ihp = (line == 13) ? 4 : line - 11;
		break;
	case BRD_QNAPTS101:
		/* map line 12-15 to EPIC IRQ0-3 */
		*ihp = line - 12;
		break;
	case BRD_SYNOLOGY:
		/* map line 13-16 to EPIC IRQ0-3 */
		*ihp = line - 13;
		break;
	default:
		/* map line 12-15 to EPIC IRQ0-3 */
		*ihp = line - 12;
		break;
	}
printf(" = EPIC %d\n", *ihp);
	return 0;
  bad:
	*ihp = -1;
	return 1;
}

const char *
pci_intr_string(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{
	static char irqstr[8];		/* 4 + 2 + NULL + sanity */

	if (ih < 0 || ih >= ICU_LEN)
		panic("pci_intr_string: bogus handle 0x%x", ih);

	sprintf(irqstr, "irq %d", ih + 16);
	return (irqstr);
	
}

const struct evcnt *
pci_intr_evcnt(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
pci_intr_establish(pc, ih, level, func, arg)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	int level, (*func) __P((void *));
	void *arg;
{
	/*
	 * ih is the value assigned in pci_intr_map(), above.
	 * It's the EPIC IRQ #.
	 */
	return intr_establish(ih + 16, IST_LEVEL, level, func, arg);
}

void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
	intr_disestablish(cookie);
}

void
pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int pin, int swiz,
    int *iline)
{
	if (bus == 0) {
		*iline = dev;
	} else {
		/*
		 * If we are not on bus zero, we're behind a bridge, so we
		 * swizzle.
		 *
		 * The documentation lies about this.  In slot 3 (numbering
		 * from 0) aka device 16, INTD# becomes an interrupt for
		 * slot 2.  INTC# becomes an interrupt for slot 1, etc.
		 * In slot 2 aka device 16, INTD# becomes an interrupt for
		 * slot 1, etc.
		 *
		 * Verified for INTD# on device 16, INTC# on device 16,
		 * INTD# on device 15, INTD# on device 13, and INTC# on
		 * device 14.  I presume that the rest follow the same
		 * pattern.
		 *
		 * Slot 0 is device 13, and is the base for the rest.
		 */
		*iline = 13 + ((swiz + dev + 3) & 3);
	}
}
