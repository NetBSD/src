/*	$NetBSD: rbus_machdep.c,v 1.13.38.1 2007/06/07 20:30:46 garbled Exp $	*/

/*
 * Copyright (c) 1999
 *     TSUBAI Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rbus_machdep.c,v 1.13.38.1 2007/06/07 20:30:46 garbled Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <powerpc/oea/bat.h>
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/cardbus/rbus.h>
#include <dev/ofw/openfirm.h>

static void macppc_cardbus_init __P((pci_chipset_tag_t, pcitag_t));

#ifdef DEBUG_ALLOC
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

int
md_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size,
    int flags, bus_space_handle_t *bshp)
{
	DPRINTF("md_space_map: %p, 0x%x, 0x%x\n", t, bpa, size);

	return bus_space_map(t, bpa, size, flags, bshp);
}

void
md_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size,
    bus_addr_t *adrp)
{
	DPRINTF("md_space_unmap: %p 0x%x\n", t, bsh);

	bus_space_unmap(t, bsh, size);
}

rbus_tag_t
rbus_pccbb_parent_mem(pa)
	struct pci_attach_args *pa;
{
	bus_addr_t start;
	bus_size_t size;
	int node, reg[5];

	macppc_cardbus_init(pa->pa_pc, pa->pa_tag);

	node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
	OF_getprop(node, "assigned-addresses", reg, sizeof(reg));

	start = reg[2];
	size = reg[4];

	/* XXX PowerBook G3 */
	if (size < 0x10000) {
		start = 0x90000000;
		size  = 0x10000000;
	}

	battable[start >> 28].batl = BATL(start, BAT_I, BAT_PP_RW);
	battable[start >> 28].batu = BATU(start, BAT_BL_256M, BAT_Vs);

	return rbus_new_root_delegate(pa->pa_memt, start, size, 0);
}

rbus_tag_t
rbus_pccbb_parent_io(pa)
	struct pci_attach_args *pa;
{
	bus_addr_t start = 0x2000;
	bus_size_t size  = 0x0800;

	start += pa->pa_function * size;
	return rbus_new_root_delegate(pa->pa_iot, start, size, 0);

	/* XXX we should use ``offset''? */
}

void
macppc_cardbus_init(pc, tag)
	pci_chipset_tag_t pc;
	pcitag_t tag;
{
	u_int x;
	static int initted = 0;

	if (initted)
		return;
	initted = 1;

	/* XXX What about other bridges? */

	x = pci_conf_read(pc, tag, PCI_ID_REG);
	if (PCI_VENDOR(x) == PCI_VENDOR_TI &&
	    PCI_PRODUCT(x) == PCI_PRODUCT_TI_PCI1211) {
		/* For CardBus card. */
		pci_conf_write(pc, tag, 0x18, 0x10010100);

		/* Route INTA to MFUNC0 */
		x = pci_conf_read(pc, tag, 0x8c);
		x |= 0x02;
		pci_conf_write(pc, tag, 0x8c, x);

		tag = pci_make_tag(pc, 0, 0, 0);
		x = pci_conf_read(pc, tag, PCI_ID_REG);
		if (PCI_VENDOR(x) == PCI_VENDOR_MOT &&
		    PCI_PRODUCT(x) == PCI_PRODUCT_MOT_MPC106) {
			/* Set subordinate bus number to 1. */
			x = pci_conf_read(pc, tag, 0x40);
			x |= 1 << 8;
			pci_conf_write(pc, tag, 0x40, x);
		}
	}
}
