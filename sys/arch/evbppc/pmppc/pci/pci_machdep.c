/*	$NetBSD: pci_machdep.c,v 1.5.12.1 2017/12/03 11:36:12 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.5.12.1 2017/12/03 11:36:12 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/intr.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <uvm/uvm.h>

#define _POWERPC_BUS_DMA_PRIVATE

#include <dev/ic/cpc700reg.h>

#include <machine/pmppc.h>
#include <machine/pio.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#include <evbppc/pmppc/dev/mainbus.h>

/*
 * Address conversion as seen from a PCI master.
 * XXX Shouldn't use 0x80000000, the actual value
 * should come from the BAR.
 */
#define PHYS_TO_PCI_MEM(x)	((x) + 0x80000000)
#define PCI_MEM_TO_PHYS(x)	((x) - 0x80000000)

static bus_addr_t phys_to_pci(bus_dma_tag_t, bus_addr_t);
static bus_addr_t pci_to_phys(bus_dma_tag_t, bus_addr_t);

extern struct powerpc_bus_dma_tag pci_bus_dma_tag;

void
pmppc_pci_get_chipset_tag(pci_chipset_tag_t pc)
{
	pc->pc_conf_v = (void *)pc;

	pc->pc_attach_hook = genppc_pci_indirect_attach_hook;
	pc->pc_bus_maxdevs = genppc_pci_bus_maxdevs;
	pc->pc_make_tag = genppc_pci_indirect_make_tag;
	pc->pc_conf_read = genppc_pci_indirect_conf_read;
	pc->pc_conf_write = genppc_pci_indirect_conf_write;

	pc->pc_intr_v = (void *)pc;

	pc->pc_intr_map = pmppc_pci_intr_map;
	pc->pc_intr_string = genppc_pci_intr_string;
	pc->pc_intr_evcnt = genppc_pci_intr_evcnt;
	pc->pc_intr_establish = genppc_pci_intr_establish;
	pc->pc_intr_disestablish = genppc_pci_intr_disestablish;
	pc->pc_intr_setattr = genppc_pci_intr_setattr;
	pc->pc_intr_type = genppc_pci_intr_type;
	pc->pc_intr_alloc = genppc_pci_intr_alloc;
	pc->pc_intr_release = genppc_pci_intr_release;
	pc->pc_intx_alloc = genppc_pci_intx_alloc;

	pc->pc_msi_v = (void *)pc;
	genppc_pci_chipset_msi_init(pc);

	pc->pc_msix_v = (void *)pc;
	genppc_pci_chipset_msix_init(pc);

	pc->pc_conf_interrupt = pmppc_pci_conf_interrupt;
	pc->pc_decompose_tag = genppc_pci_indirect_decompose_tag;
	pc->pc_conf_hook = genppc_pci_conf_hook;

	pc->pc_addr = mapiodev(CPC_PCICFGADR, 4, false);
	pc->pc_data = mapiodev(CPC_PCICFGDATA, 4, false);
	pc->pc_bus = 0;
	pc->pc_node = 0;
	pc->pc_memt = 0;
	pc->pc_iot = 0;

	/* the following two lines are required because unlike other ports, 
	 * we cannot just add PHYS_TO_BUS_MEM/BUS_MEM_TO_PHYS defines to
	 * bus.h, because it would impact other evbppc ports.
	 */
	pci_bus_dma_tag._dma_phys_to_bus_mem = phys_to_pci;
	pci_bus_dma_tag._dma_bus_mem_to_phys = pci_to_phys;
}


static bus_addr_t
phys_to_pci(bus_dma_tag_t t, bus_addr_t a)
{
	return PHYS_TO_PCI_MEM(a);
}

static bus_addr_t pci_to_phys(bus_dma_tag_t t, bus_addr_t a)
{
	return PCI_MEM_TO_PHYS(a);
}

int
pmppc_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
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

	if (line == 255) {
		printf("pci_intr_map: no mapping for pin %c\n", '@' + pin);
		goto bad;
	}
	/*printf("pci_intr_map pin=%d line=%d\n", pin, line);*/

	switch (line & 3) {	/* XXX what should this be? */
	case 0: *ihp = PMPPC_I_BPMC_INTA; break;
	case 1: *ihp = PMPPC_I_BPMC_INTB; break;
	case 2: *ihp = PMPPC_I_BPMC_INTC; break;
	case 3: *ihp = PMPPC_I_BPMC_INTD; break;
	}
	return 0;

bad:
	*ihp = -1;
	return 1;
}

void
pmppc_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{
	int line;

	line = (swiz + dev) & 3;
	/* XXX UGLY UGLY, figure out the real interrupt mapping */
	if (bus==3&&dev==2&&pin==1&&swiz==3) line=2;
/*
	printf("pci_conf_interrupt: bus=%d dev=%d pin=%d swiz=%d => line=%d\n",
		bus, dev, pin, swiz, line);
*/
	*iline = line;
}
