/*	$NetBSD: pci_machdep.c,v 1.7 1999/02/04 14:54:00 tsubai Exp $	*/

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
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#define _MACPPC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

/*
 * PCI doesn't have any special needs; just use the generic versions
 * of these functions.
 */
struct macppc_bus_dma_tag pci_bus_dma_tag = {
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
	return 32;
}

pcitag_t
pci_make_tag(pc, bus, device, function)
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	tag = 0x80000000 | (bus << 16) | (device << 11) | (function << 8);

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

pcireg_t
pci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	pcireg_t data;
	struct pci_bridge *r;

	if (pc == PCI_CHIPSET_MPC106) {
		r = &pci_bridges[0];

		out32rb(r->addr, tag | reg);
		data = in32rb(r->data);
		out32rb(r->addr, 0);
	} else {
		int bus, dev, func;

		pci_decompose_tag(pc, tag, &bus, &dev, &func);

		r = &pci_bridges[pc];

		/*
		 * bandit's minimum device number of the first bus is 11.
		 * So we behave as if there is no device when dev < 11.
		 */
		if (func > 7)
			panic("pci_conf_read: func > 7");

		if (bus == r->bus) {
			if (dev < 11) {
				if (reg == PCI_ID_REG)
					return 0xffffffff;
				else
					panic("pci_conf_read: dev < 11");
			}
			out32rb(r->addr, (1 << dev) | (func << 8) | reg);
		} else
			out32rb(r->addr, tag | reg | 1);
		DELAY(10);
		data = in32rb(r->data);
		DELAY(10);
		out32rb(r->addr, 0);
		DELAY(10);
	}

	return data;
}

void
pci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	struct pci_bridge *r = &pci_bridges[pc];

	if (pc == PCI_CHIPSET_MPC106) {
		r = &pci_bridges[0];

		out32rb(r->addr, tag | reg);
		out32rb(r->data, data);
		out32rb(r->addr, 0);
	} else {
		int bus, dev, func;

		pci_decompose_tag(pc, tag, &bus, &dev, &func);
		r = &pci_bridges[pc];

		if (func > 7)
			panic("pci_conf_write: func > 7");

		if (bus == r->bus) {
			if (dev < 11)
				panic("pci_conf_write: dev < 11");
			out32rb(r->addr, (1 << dev) | (func << 8) | reg);
		} else
			out32rb(r->addr, tag | reg | 1);
		DELAY(10);
		out32rb(r->data, data);
		DELAY(10);
		out32rb(r->addr, 0);
		DELAY(10);
	}
}

int
pci_intr_map(pc, intrtag, pin, line, ihp)
	pci_chipset_tag_t pc;
	pcitag_t intrtag;
	int pin, line;
	pci_intr_handle_t *ihp;
{

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
	if (line == 0 || line == 255) {
		printf("pci_intr_map: no mapping for pin %c\n", '@' + pin);
		goto bad;
	} else {
		if (line >= ICU_LEN) {
			printf("pci_intr_map: bad interrupt line %d\n", line);
			goto bad;
		}
	}

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

	if (ih == 0 || ih >= ICU_LEN)
		panic("pci_intr_string: bogus handle 0x%x\n", ih);

	sprintf(irqstr, "irq %d", ih);
	return (irqstr);
	
}

extern void * intr_establish();
extern void intr_disestablish();

void *
pci_intr_establish(pc, ih, level, func, arg)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	int level, (*func) __P((void *));
	void *arg;
{

	if (ih == 0 || ih >= ICU_LEN)
		panic("pci_intr_establish: bogus handle 0x%x\n", ih);

	return intr_establish(ih, IST_LEVEL, level, func, arg);
}

void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{

	intr_disestablish(cookie);
}
