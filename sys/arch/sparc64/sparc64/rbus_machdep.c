/*	$NetBSD: rbus_machdep.c,v 1.14.2.1 2009/05/13 17:18:38 jym Exp $	*/

/*
 * Copyright (c) 2003 Takeshi Nakayama.
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
__KERNEL_RCSID(0, "$NetBSD: rbus_machdep.c,v 1.14.2.1 2009/05/13 17:18:38 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/openfirm.h>
#include <machine/promlib.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/ppbreg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychovar.h>

#include <dev/cardbus/rbus.h>
#include <dev/pcmcia/pcmciachip.h>
#include <dev/ic/i82365reg.h>
#include <dev/pci/pccbbreg.h>
#include <dev/pci/pccbbvar.h>

#ifdef RBUS_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

static int pccbb_cardbus_isvalid(void *);

int
md_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags,
	     bus_space_handle_t *bshp)
{
	DPRINTF("md_space_map: 0x%" PRIxPTR ", 0x%" PRIx64 ", 0x%" PRIx64 "\n",
		(u_long)t->cookie, bpa, size);

	return bus_space_map(t, bpa, size, flags, bshp);
}

void
md_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size, bus_addr_t *adrp)
{
	DPRINTF("md_space_unmap: 0x%" PRIxPTR ", 0x%" PRIx64 ", 0x%" PRIx64
		"\n", (u_long)t->cookie, bsh._ptr, size);

	/* return the PCI offset address if required */
	if (adrp != NULL)
		*adrp = psycho_bus_offset(t, &bsh);
	bus_space_unmap(t, bsh, size);
}

rbus_tag_t
rbus_pccbb_parent_mem(struct pci_attach_args *pa)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	struct psycho_pbm *pp = pc->cookie;
	struct extent *ex = pp->pp_exmem;
	bus_addr_t start;
	bus_size_t size;

	if (ex == NULL)
		panic("rbus_pccbb_parent_mem: extent is not initialized");

	start = ex->ex_start;
	size = ex->ex_end - start;

	return rbus_new_root_share(pa->pa_memt, ex, start, size, 0);
}

rbus_tag_t
rbus_pccbb_parent_io(struct pci_attach_args *pa)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	struct psycho_pbm *pp = pc->cookie;
	struct extent *ex = pp->pp_exio;
	bus_addr_t start;
	bus_size_t size;

	if (ex == NULL)
		panic("rbus_pccbb_parent_io: extent is not initialized");

	start = ex->ex_start;
	size = ex->ex_end - start;

	return rbus_new_root_share(pa->pa_iot, ex, start, size, 0);
}

/*
 * Machine dependent part for the attachment of PCI-CardBus bridge.
 * This function is called from pccbb_attach() in sys/dev/pci/pccbb.c.
 */
void
pccbb_attach_hook(device_t parent, device_t self, struct pci_attach_args *pa)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcireg_t reg;
	int node = PCITAG_NODE(pa->pa_tag);
	int error;
	int bus, br[2], *brp;
	int len, intr;

	/*
	 * bus fixup:
	 *	if OBP didn't assign a bus number to the cardbus bridge,
	 *	then assign it here.
	 */
	brp = br;
	len = 2;
	error = prom_getprop(node, "bus-range", sizeof(*brp), &len, &brp);
	if (error == 0 && len == 2) {
		bus = br[0];
		DPRINTF("pccbb_attach_hook: bus-range %d-%d\n", br[0], br[1]);
		if (bus < 0 || bus >= 256)
			printf("pccbb_attach_hook: broken bus %d\n", bus);
		else {
#ifdef DIAGNOSTIC
			if ((*pc->spc_busnode)[bus].node != 0)
				printf("pccbb_attach_hook: override bus %d"
				       " node %08x -> %08x\n",
				       bus, (*pc->spc_busnode)[bus].node, node);
#endif
			(*pc->spc_busnode)[bus].arg = device_private(self);
			(*pc->spc_busnode)[bus].valid = pccbb_cardbus_isvalid;
			(*pc->spc_busnode)[bus].node = node;
		}
	} else {
		bus = ++pc->spc_busmax;
		DPRINTF("pccbb_attach_hook: bus %d\n", bus);
		if (bus >= 256)
			printf("pccbb_attach_hook: 256 >= busses exist\n");
		else {
			reg = pci_conf_read(pc, pa->pa_tag, PPB_REG_BUSINFO);
			reg &= 0xff000000;
			reg |= pa->pa_bus | (bus << 8) | (bus << 16);
			pci_conf_write(pc, pa->pa_tag, PPB_REG_BUSINFO, reg);
#ifdef DIAGNOSTIC
			if ((*pc->spc_busnode)[bus].node != 0)
				printf("pccbb_attach_hook: override bus %d"
				       " node %08x -> %08x\n",
				       bus, (*pc->spc_busnode)[bus].node, node);
#endif
			(*pc->spc_busnode)[bus].arg = device_private(self);
			(*pc->spc_busnode)[bus].valid = pccbb_cardbus_isvalid;
			(*pc->spc_busnode)[bus].node = node;
		}
	}

	/*
	 * interrupt fixup:
	 *	fake interrupt line not for giving up the probe.
	 *	interrupt numbers assigned by OBP are [0x00,0x3f],
	 *	so they map to [0x40,0x7f] due to inhibit the value 0x00.
	 */
	if ((intr = prom_getpropint(node, "interrupts", -1)) == -1) {
		printf("pccbb_attach_hook: could not read interrupts\n");
		return;
	}

	if (OF_mapintr(node, &intr, sizeof(intr), sizeof(intr)) < 0) {
		printf("pccbb_attach_hook: OF_mapintr failed\n");
		return;
	}

	pa->pa_intrline = intr | 0x40;
	DPRINTF("pccbb_attach_hook: interrupt line %d\n", intr);
}

/*
 * Detect a validity of CardBus bus, since it occurs PCI bus error
 * when a CardBus card is not present or power-off.
 */
static int
pccbb_cardbus_isvalid(void *arg)
{
	struct pccbb_softc *sc = arg;
	bus_space_tag_t memt = sc->sc_base_memt;
	bus_space_handle_t memh = sc->sc_base_memh;
	uint32_t sockstat, sockctrl;

	/* check CardBus card is present */
	sockstat = bus_space_read_4(memt, memh, CB_SOCKET_STAT);
	DPRINTF("%s: pccbb_cardbus_isvalid: sockstat %08x\n",
		device_xname(sc->sc_dev), sockstat);
	if ((sockstat & CB_SOCKET_STAT_CB) == 0 ||
	    (sockstat & CB_SOCKET_STAT_CD) != 0)
		return 0;

	/* check card is powered on */
	sockctrl = bus_space_read_4(memt, memh, CB_SOCKET_CTRL);
	DPRINTF("%s: pccbb_cardbus_isvalid: sockctrl %08x\n",
		device_xname(sc->sc_dev), sockctrl);
	if ((sockctrl & CB_SOCKET_CTRL_VCCMASK) == 0)
		return 0;

	/* card is present and powered on */
	return 1;
}
