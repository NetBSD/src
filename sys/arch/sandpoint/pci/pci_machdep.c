/*	$NetBSD: pci_machdep.c,v 1.5 2001/08/20 12:20:06 wiz Exp $	*/

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

#include <sandpoint/isa/icu.h>

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

#define	PCI_CONFIG_ENABLE	0x80000000UL

void
pci_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
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
	} else {
		/*
		 * Sandpoint has 4 PCI slots.
		 * Sandpoint rev. X2 has them in a weird order.  Counting
		 * from center out toward the edge, we have:
		 * 	Slot 1 (dev 14?) (labelled 1)
		 * 	Slot 0 (dev 13?) (labelled 2)
		 * 	Slot 3 (dev 16)  (labelled 3)
		 * 	Slot 2 (dev 15)  (labelled 4)
		 * To keep things confusing, we will consistently use a zero-
		 * based numbering scheme where Motorola's is usually 1-based.
		 */
		if (line < 13 || line > 16) {
			printf("pci_intr_map: bad interrupt line %d\n", line);
			goto bad;
		}
	}
	/*
	 * In the PCI configuration code, we simply assign the dev
	 * number to the interrupt line.  We extract it here for the
	 * interrupt, but subtract off the lowest dev (13) to get
	 * the IRQ.
	 */
	line -= 13;
	
	*ihp = line;
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
		panic("pci_intr_string: bogus handle 0x%x\n", ih);

	sprintf(irqstr, "irq %d", ih);
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
	if (ih < 0 || ih >= 4)
		panic("pci_intr_establish: bogus handle 0x%x\n", ih);

	/*
	 * ih is the value assigned in pci_intr_map(), above.
	 * For the Sandpoint, this is the zero-based slot #,
	 * configured when the bus is set up.
	 */
	return intr_establish(ih, IST_LEVEL, level, func, arg);
}

void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
	intr_disestablish(cookie);
}

void
pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int func, int swiz,
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
